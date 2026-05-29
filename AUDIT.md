# AUDIT: RetroWorld Engine

> Дата: 2026-05-28
> Версия кода: текущая (1 файл GameApp.cpp выключен из билда)
> Анализатор: ручной, по исходному коду

---

## 1. КРИТИЧЕСКИЕ БАГИ (Crash / Data Corruption)

### 1.4 `stbi_load` — нет обработки ошибок

**Файл:** `src/Engine/gpu_texture.cpp:247-262`
```cpp
const auto imageData = stbi_load(path.data(), &x, &y, nullptr, 4);
assert(imageData); // TODO:
```
**Проблема:** если файл не найден или повреждён, в `Release` сборке `imageData == nullptr`, `CreateTexture2D` получит `extent = {0,0}`, последующий `glTextureStorage2D` с нулевыми размерами — UB.
**Исправление:** проверять возврат и логировать ошибку.

### 1.5 `UpdateData` — каст к `GLuint` для offset/size на 64-бит

**Файл:** `src/Engine/gpu_buffer.cpp:137`
```cpp
glNamedBufferSubData(buffer->id, static_cast<GLuint>(destOffsetBytes), static_cast<GLuint>(size), data);
```
**Проблема:** на 64-битной системе буфер может быть >4GB, `GLuint` обрежет offset/size. Сейчас буферы <4GB, но это скрытая мина.
**Исправление:** использовать `GLintptr`/`GLsizeiptr`.

### 1.6 `infoLog.resize(maxLength - 1)` — потенциальный buffer overflow

**Файл:** `src/Engine/gpu_program.cpp:119-123`
```cpp
infoLog.resize(static_cast<size_t>(maxLength - 1));
glGetShaderInfoLog(shader, maxLength, nullptr, infoLog.data());
```
**Проблема:** `infoLog` имеет размер `maxLength-1` байт, но `glGetShaderInfoLog` получает `maxLength` как `bufSize` — пишет до `maxLength` байт. Выход за границу на 1 байт (плюс null может быть на позиции `maxLength`, что ещё на 1 байт дальше).
**Воспроизводится:** при `maxLength == 2` (очень короткая ошибка компиляции — маловероятно, но возможно).
**Исправление:** `infoLog.resize(maxLength)`.

---

## 2. ПРОИЗВОДИТЕЛЬНОСТЬ (Hot-path, аллокации, driver overhead)

### 2.1 `Traverse()` через `std::function` — аллокация каждый кадр

**Файл:** `src/Engine/sc_node.cpp:39`
```cpp
void scene::SceneNode::Traverse(const std::function<void(SceneNode&, const glm::mat4&)>& fn)
```
**Проблема:** `std::function` может аллоцировать хип при захвате лямбды с большим количеством переменных или с move-only объектами. Вызывается каждый кадр для scene graph.
**Альтернатива:** шаблонный параметр `typename Fn` или тип-эрейзер с small buffer optimization (например, `function_ref`, `yap::function_ref`).

### 2.2 `BlitTexture` — создаёт временные FBO каждый вызов

**Файл:** `src/Engine/gpu_cmd.cpp:50-85`
```cpp
inline gpu::fbo::FramebufferPtr makeSingleTextureFbo(gpu::texture::TexturePtr texture)
```
**Проблема:** при каждом blit создаются 1-2 новых `Framebuffer` (shared_ptr с аллокацией + GL state). Если blit вызывается часто, это лишний driver overhead.
**Исправление:** кэшировать FBO по texture ID (уже есть `framebufferCacheKey`/`framebufferCacheValue` в `ContextState`, но не используется для blit).

### 2.3 SSBO recreation проверяется дважды за кадр

**Файлы:** `sc_sceneManager.cpp:372-379` и `sc_sceneManager.cpp:703-709`
**Проблема:** и `RenderShadowPass`, и `drawRenderItem` проверяют `count > m_instanceCapacity` и при необходимости пересоздают SSBO. Если shadow pass увеличил capacity, opaque pass увидит уже достаточный размер, но проверка всё равно выполняется.
**Оптимизация:** вынести проверку/расширение SSBO в одно место перед основным проходом рендера.

### 2.4 `GLEnableOrDisable` — лишние вызовы `glEnable`/`glDisable` в hot-path

**Файлы:** `gpu_cmd.cpp:6-9`, `gpu_cmd.cpp:346-348`, `gpu_cmd.cpp:357-358`, `gpu_cmd.cpp:372-378`, etc.
**Проблема:** каждый кадр устанавливается DepthState, RasterizationState и т.д. — большинство значений не меняется между кадрами. `ContextState` сравнивает значения, но для state, которые не меняются от кадра к кадру (например, depthClampEnable), всё равно происходит сравнение + conditional branch.
**Статус:** архитектурно приемлемо, но стоит добавить dirty-флаги на уровне «теневого состояния» между кадрами.

### 2.5 Вычисление `normalMatrix` в вертексном шейдере для instanced

**Файл:** `GameApp.cpp:29` (GLSL)
```glsl
mat3 normalMatrix = u_isInstanced ? transpose(inverse(mat3(model))) : u_normalMatrix;
```
**Проблема:** `transpose(inverse(mat3(model)))` — очень дорогая операция для каждого вертекса (3x3 inverse + transpose в шейдере). Правильнее передавать normal matrix через SSBO вместе с model matrix, либо вычислять на CPU.
**Исправление:** добавить `mat3 normalMatrix` в SSBO `InstanceBuffer` (занять 3 vec4, добавив padding к struct из 4 vec4) — будет 16 float на инстанс вместо 16.

### 2.6 Wireframe state restore — лишний draw call overhead

**Файл:** `sc_sceneManager.cpp:471-473`
```cpp
if (wireframeMode)
    gpu::cmd::SetState(m_fillState);
```
**Проблема:** `SetState()` сравнивает всё состояние и вызывает `glPolygonMode` и т.д. даже если wireframe не включен. Но если включен — делается restore в fill режим после opaue pass, а transparent pass (если есть) снова переключит в fill (если не wireframe). Это корректно, но лучше не делать restore вовсе, а переключать wireframe через отдельный флаг/препасс.

---

## 3. ДИЗАЙН И АРХИТЕКТУРА

### 3.1 Инлайн шейдеров в C++ строки

**Файл:** `GameApp.cpp` (все шейдеры)
**Проблема:** шейдеры хранятся как строковые литералы прямо в `GameApp.cpp`. Это усложняет отладку (нет подсветки синтаксиса, нет валидации на уровне IDE), требует перекомпиляции C++ при любом изменении шейдера, мешает использованию внешних тулов (shader analyzers).
**Рекомендация:** вынести в `.glsl` файлы, загружать через `LoadShaderCode` (уже есть!).

### 3.2 Единый `LightBlockUBO` с фиксированным максимумом в 16 источников

**Файл:** `sc_sceneManager.cpp:30-33`
```cpp
struct alignas(16) LightBlockUBO
{
    int32_t      lightCount;
    uint8_t      _pad[12];
    LightDataGPU lights[16];
};
```
**Проблема:** 16 источников * 144 байта = 2304 байта UBO. Это влезает в гарантированный минимум UBO size (16384 для GL 4.5), но:
1. При >16 источниках света — silent overflow.
2. Пассивная передача 144 байт × 16 = 2.3KB каждый кадр, даже если реально 1-2 источника.
**Рекомендация:** перейти на SSBO для динамического числа источников.

### 3.4 `SceneManager::lights` — сырые указатели, нет проверки на удаление ноды

**Файл:** `sc_sceneManager.h:77`
```cpp
std::vector<LightNode*> lights; // raw pointers, collected each frame
```
**Проблема:** если `LightNode` удаляется из сцены, указатель в векторе становится висячим. Сейчас `lights` пересобирается каждый кадр в `Update()`, поэтому удаление между `Update` и `UploadLights` — единственный опасный сценарий. Стоит добавить защиту или использовать `weak_ptr`.

### 3.5 `MeshMaterialPair` — сравнение сырых указателей

**Файл:** `sc_sceneManager.cpp:35-40`
```cpp
struct MeshMaterialPair
{
    const gr::Mesh* mesh;
    const gr::Material* material;
    bool operator==(const MeshMaterialPair&) const = default;
};
```
**Проблема:** сравнение по адресам корректно только если mesh/material живут в памяти и не пересоздаются между кадрами. `shared_ptr` гарантирует стабильность адреса. Допустимо, но хрупко.

### 3.6 `SceneNode::virtual ~SceneNode()` — полиморфизм через виртуальные функции

**Файл:** `sc_node.h:22`
```cpp
virtual ~SceneNode() = default;
```
**Замечание:** по правилам проекта виртуальные функции запрещены в hot-path. Деструктор не в hot-path, так что это ок. Однако если не планируется наследование за пределами 4-х известных типов, можно заменить на `protected` non-virtual деструктор и явный `type`-based deleter.

### 3.8 `ShadowMapManager` — не очищается при смене light-нод

**Файл:** `gr_shadowMapManager.cpp` (требуется проверка)
**Риск:** если `LightNode` удалён, его `ShadowMap` остаётся в `ShadowMapManager::m_shadowMaps` (мап по сырому указателю). Указатель становится висячим. `GetOrCreate` вернёт старую карту для нового light на том же адресе, что маловероятно, но возможно.

---

## 4. OPENGL SPECIFIC

### 4.3 `glNamedBufferSubData` с `GLuint` кастом

**Файл:** `gpu_buffer.cpp:137`
Уже описано в 1.5. Дополнительно: не проверяется наличие `GL_ARB_direct_state_access` через `GL_EXT_direct_state_access`. Для 4.5 core DSA — обязателен.

### 4.5 Непроверяемый вызов `glTextureView` с несовместимым форматом

**Файл:** `gpu_texture.cpp:271-278`
Нет проверки, что `viewInfo.format` совместим с оригинальным форматом (требуется совпадение internal format class). При несовместимости — `GL_INVALID_OPERATION`.

### 4.6 Нет `glMemoryBarrier` при использовании SSBO между draw calls

**Файл:** `gpu_cmd.cpp` — `Draw()`/`DrawIndexed()` не вставляют барьеры. Если SSBO пишется в одном проходе и читается в другом (например, indirect compute → draw), нужен `glMemoryBarrier`. Сейчас SSBO используется read-only в шейдере, так что это не проблема (пока).

---

## 5. КОДСТАЙЛ И КОНСИСТЕНТНОСТЬ

### 5.2 `MouseLook::BeginCapture` дублирует установку позиции мыши

**Файл:** `GameApp.cpp:364-374`
```cpp
math::point2 current = input::GetMousePosition();
m_centerX = current.x;
m_centerY = current.y;
input::CaptureMause(true);
input::SetMousePosition(m_centerX, m_centerY);
```
Позиция мыши уже равна `current`, так что `SetMousePosition` — избыточен (но и безвреден).

### 5.3 Комментарии на русском в коде

**Файлы:** `gpu_program.cpp:35`, `app.cpp:86,92,104`
```cpp
// поддержка Windows
// Расчёт deltaTime
// Защита от экстремальных значений (лагов, паузы)
```
Incosistent с проектом (AGENTS.md не запрещает, но смесь языков усложняет международную разработку).

### 5.4 `std::unreachable()` в Release без `=default`

**Файлы:** `gpu_cmd.cpp:114,136,159,200,227,250,255,424`, `gpu_texture.cpp:59,473,604`, `gpu_program.cpp:65,185`
В `default` ветках switch используется `std::unreachable()`. Корректно для C++23.

### 5.5 Нет `noexcept` на большинстве функций

**Ситуация:** `gpu::cmd::BindFramebuffer`, `gpu::cmd::SetState` и т.д. не помечены `noexcept`. По правилам AGENTS.md «явный noexcept на всех функциях, где возможно». Большинство из них может быть `noexcept`.
**Исключение:** функции, которые могут аллоцировать (std::vector push_back, std::make_shared).

### 5.6 `gpu_copy.h/cpp` — есть, но не используются

**Файл:** `src/Engine/gpu_copy.h`, `gpu_copy.cpp`
Реализация функций copy buffer/texture. Не включена ни в один проход рендера. Либо dead code, либо подготовка для будущего использования.

### 5.7 `gpu_fence.h/cpp` и `gpu_timer.h/cpp` — есть, не используются в hot-path

Это нормально — это инфраструктура для будущих фич (профилирование, async compute).

### 5.8 `gpu_uniform.h` — `Uniform<T>` template есть, не используется

**Файл:** `gpu_uniform.h`
Шаблон для type-safe uniform binding. SceneManager использует прямые вызовы `SetUniform` с location. `Uniform<T>` — dead code или задел.

---

## 6. БЕЗОПАСНОСТЬ И УСТОЙЧИВОСТЬ

### 6.1 Нет обработки потери OpenGL контекста

При переключении fullscreen ↔ windowed, смене разрешения, спящем режиме — контекст может быть разрушен. Все GL ресурсы (VBO, VAO, texture IDs) станут инвалидны. Нет никакого механизма пересоздания.

### 6.2 `UpdateData` не синхронизируется с GPU

**Файл:** `gpu_buffer.cpp:137`
`glNamedBufferSubData` для буфера с `DynamicStorage` не гарантирует, что предыдущий draw, читающий из этого буфера, завершился. Теоретически возможна гонка: GPU читает старые данные, CPU пишет новые. На практике для `DynamicStorage` драйвер обычно делает копию + синхронизацию неявно. Но при активном использовании SSBO (instance transforms, которые обновляются каждый кадр) — риск есть.
**Рекомендация:** использовать орбитальные буферы (double/triple buffering) для instance data.

### 6.3 `assert` в `gpu_cmd.cpp` не срабатывает в Release

**Файлы:** `gpu_cmd.cpp` — все `assert(context.isRendering)`.
В Release `assert` убирается. Если по ошибке вызвать GPU команду вне фрейма — будет тихий UB.
**Рекомендация:** сделать compile-time опцию для validation в Release (через макрос `SE_GPU_VALIDATION`).

---

## 7. ТЕСТИРУЕМОСТЬ

### 7.1 Нет unit-тестов

**Файл:** `src/Engine/mathTest.cpp` — пустой.
Весь проект не покрыт тестами. AABB, Frustum, Transform — критически важные для корректности рендера части без тестов.

### 7.2 Нет тестов для шейдеров

Шейдеры валидируются только при запуске игры. Нет offline проверки синтаксиса, нет pipeline-specific тестов.

---

## 8. МЕЛКИЕ ЗАМЕЧАНИЯ

### 8.1 `light.positionOrDirection.w` = type в шейдере C++ дублирует `data.type`

Не баг (и shader и CPU согласованы), но избыточно.

### 8.2 `FindChild("cube")` всегда возвращает nullptr

**Файл:** `GameApp.cpp:535`
```cpp
auto* cube = g_scene->root->FindChild("cube");
```
Кубы названы `cube_0`..`cube_8`. Этот поиск всегда nullptr. Код анимации закомментирован — но сам поиск остался.

### 8.3 `int cascadeCount` — не используется как реальное число каскадов

**Файл:** `sc_sceneManager.cpp:404-421`
`cascadeCount` вычисляется, но свет с `cascadeDistance[0] = -1` (GameApp:486) даёт `cascadeCount = 0`, а затем `cascadeCount = 1` на строке 407. Затеняющее присваивание.

### 8.4 VertexArray cache в ContextState не чистится

**Файл:** `_gpu_contextState.h:97`
```cpp
std::unordered_map<size_t, gpu::vao::VertexArrayPtr> vertexArrayCache;
```
Кэш растёт бесконечно — при динамическом создании VAO (например, в procedural mesh-генерации) количество записей будет увеличиваться каждый кадр. Нет механизма очистки.

---

## ИТОГ

| Категория | Всего | Critical | High | Medium | Low |
|-----------|-------|----------|------|--------|-----|
| Баги | 6 | 3 | 1 | 1 | 1 |
| Производительность | 6 | 0 | 2 | 2 | 2 |
| Дизайн | 9 | 0 | 3 | 3 | 3 |
| OpenGL | 6 | 0 | 2 | 2 | 2 |
| Кодстайл | 8 | 0 | 0 | 2 | 6 |
| Безопасность | 3 | 0 | 2 | 1 | 0 |
| Тестирование | 2 | 0 | 1 | 1 | 0 |
| **Итого** | **40** | **3** | **11** | **12** | **14** |

### Топ-5 что чинить в первую очередь:
3. **`stbi_load` без обработки ошибок** (1.4)
4. **`Traverse()` через `std::function` — хип-аллокация каждый кадр** (2.1)
5. **`infoLog.resize` — buffer overflow** (1.6)
