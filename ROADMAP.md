# ROADMAP: RetroWorld → First-Person RPG Engine

> Цель: движок для RPG от первого лица в стиле Daggerfall (ретро-3D, процедурная генерация, открытый мир)

---

## 🔴 Фаза 0: Подготовка (1-2 недели)

Исправить критические баги из AUDIT.md прежде чем писать новый код.

- [ ] Каскадные тени + instancing (AUDIT 1.1)
- [ ] `GetCreateInfo(nullptr)` UB (AUDIT 1.3)
- [ ] `stbi_load` без обработки ошибок (AUDIT 1.4)
- [ ] `infoLog.resize` buffer overflow (AUDIT 1.6)
- [ ] `UpdateData` GLuint truncation (AUDIT 1.5)
- [ ] `Traverse()` переписать на шаблон (AUDIT 2.1)
- [ ] Выключить `glClipControl(GL_LOWER_LEFT)` (AUDIT 4.2)
- [ ] Включить `glDebugMessageCallback` (AUDIT 4.1)

---

## 🟠 Фаза 1: Террейновая система (3-4 недели)

Сердце Daggerfall-подобного мира. Нужна процедурная генерация местности.

### 1.1 Chunk-based террейн
- `TerrainChunk` — 64×64 или 128×128 тайлов, LOD
- `TerrainManager` — пул чанков, стриминг вокруг камеры
- Heightmap: генерация через симплекс-шум + erosion
- `Mesh::CreateTerrain(heightmap, size)` — триангуляция
- Материал для террейна (triplanar texturing)

### 1.2 Блоковый/воксельный мир (опционально)
- `BlockWorld` — 3D массив блоков (16×16×16), как в Daggerfall
- `BlockNode` — модель, сгенеренная из блоков (greedy meshing)
- `ChunkManager` — загрузка/выгрузка чанков

### 1.3 Вода
- `WaterNode` — плоский динамический меш с reflection/refraction
- Подводная камера (fog, цвет воды)

---

## 🟡 Фаза 2: Персонаж и экшн (2-3 недели)

### 2.1 First-person controller
- Вынести управление из `GameApp.cpp` в `PlayerController`:
  - Передвижение (WASD + прыжок + приседание)
  - Столкновения (AABB vs террейн/блоки)
  - Гравитация, скорость, ускорение
- `PhysicsWorld` — базовая физика:
  - Swept AABB collision
  - Raycast (для взаимодействия)

### 2.2 Взаимодействие (инвентарь)
- `Interactable` — компонентный интерфейс для объектов
- Pickup, открытие дверей, активация рычагов
- UI инвентаря (ImGui / собственный рендер)

### 2.3 Оружие / инструменты
- Hand slot (меч, топор, посох)
- Анимация атаки (скелетальная, если будет)
- Raycast-based hit detection

---

## 🟢 Фаза 3: Скринговый рендеринг (3-4 недели)

### 3.1 Deferred shading
- G-buffer: albedo, normal, metallic/roughness, depth
- Light pass: directional + point + spotlight, clustered
- PBR shading вместо Blinn-Phong

### 3.2 Пост-процессинг
- HDR + Bloom
- Tone mapping (ACES / Uncharted2)
- SSAO / HBAO
- Screen-space reflections (опционально)

### 3.3 Небо и погода
- Skybox / SkySphere
- Time of day cycle (солнце двигается, меняется color)
- Облака (частицы или procedural texture)
- Дождь/снег (particle system)

---

## 🔵 Фаза 4: Игровой контент (долгосрочно)

### 4.1 Система диалогов
- Dialogue trees
- NPC: простой AI (патруль, реакция на игрока)
- Quest journal

### 4.2 Процедурная генерация
- Города/деревни: размещение зданий, дорог
- Подземелья: процедурные данжены (например, BSP)
- Генерация NPC и предметов

### 4.3 Сэйвы / загрузка
- Сериализация всей сцены (scene graph + компоненты)
- Бинарный формат / JSON

---

## 🟣 Фаза 5: Оптимизация и полировка

- Frustum culling на уровне чанков
- Occlusion culling (software rasterization / hardware queries)
- GPU-driven pipeline: indirect draw, culling compute shaders
- Custom allocators (stack allocator, frame allocator, pool allocator)
- Рефакторинг `shared_ptr` на handle-based ресурсы

---

## Архитектурные рекомендации

### Сейчас (GameApp.cpp as monolith)
```
GameApp.cpp — содержит:
  - GLSL шейдеры
  - scene setup
  - input handling
  - mouselook
  - render pipeline
  → ~600 строк монолита
```

### Должно быть (модульная архитектура)
```
Game/
  PlayerController.h/cpp    — управление, физика, collision
  PlayerCamera.h/cpp        — от FPP камеры
  
World/
  TerrainChunk.h/cpp        — чанк террейна
  TerrainManager.h/cpp      — стриминг, LOD
  BlockChunk.h/cpp          — воксельный чанк
  WaterNode.h/cpp           — вода
  
Rendering/
  DeferredPipeline.h/cpp    — G-buffer + light pass
  PostProcessing.h/cpp      — bloom, tonemap
  SkySystem.h/cpp           — небо, погода
  ParticleSystem.h/cpp      — эффекты
  
UI/
  InventoryUI.h/cpp          
  DialogueUI.h/cpp
  HUD.h/cpp

Engine/ (добавить)
  PhysicsWorld.h/cpp        — AABB collision, raycast
  AudioSystem.h/cpp         — OpenAL / XAudio2
  AssetManager.h/cpp        — кэш текстур/моделей
  SaveSystem.h/cpp          — сериализация
```

---

## Технические решения для FPP RPG

### Механики, которых не хватает сейчас
| Фича | Приоритет | Зависит от |
|------|-----------|------------|
| Collision detection | 🔴 P0 | — |
| Gravity | 🔴 P0 | — |
| Raycast (block interaction) | 🔴 P0 | — |
| Deferred shading | 🟡 P1 | Фаза 3 |
| PBR pipeline | 🟡 P1 | Deferred shading |
| Procedural terrain | 🟠 P1 | Chunk system |
| Inventory | 🟠 P2 | UI system |
| Dialogue system | 🟢 P2 | — |
| Скелетная анимация | 🟢 P2 | GLTF loader (уже есть) |
| Audio | 🟢 P2 | OpenAL |

### Ключевая метрика
Для Daggerfall-стиля нужно **~5000 draw calls/кадр** при ~60 FPS на средне-бюджетном железе.
Текущий движок: ~10-20 draw calls.

**Что апнуть:**
- GPU instancing (уже есть — но только для одинаковых mesh+material)
- Multi-draw indirect (уже есть функции — не используются)
- Frustum culling на чанках (а не на отдельных моделях)
- Occlusion culling

---

## Заключение

**Текущее состояние:** крепкий фундамент (GPU abstraction, scene graph, material system, shadow mapping). 
**Слабое место:** GameApp.cpp — монолит, FPP контроллер (нет физики), воксельного/террейнового рендера нет.

### Что делать прямо сейчас
1. Исправить критические баги (Фаза 0)
2. Выделить `PlayerController` из `GameApp.cpp`
3. Реализовать swept AABB collision + гравитацию
4. Построить `TerrainChunk` с noise-based heightmap
5. Порт Blinn-Phong в deferred shading

> Следующий milestone — ходячий игрок по процедурной местности с collision. Всё остальное надстраивается на этом.
