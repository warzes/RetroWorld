#include "stdafx.h"
//=============================================================================
#if defined(_MSC_VER)
#	pragma comment( lib, "3rdparty.lib" )
#	pragma comment( lib, "Engine.lib" )
#endif
// в этом видео по дагеру https://www.youtube.com/watch?v=4Lz5d-g4tik
// визуальный стиль замка в первом отрывке - вот мне надо такую графикую
// визуальный стиль https://gruno-kromer.itch.io/snake3d-hakodate-city
//мир строится из блоков.пример редактора - halftimber
//также примеры - это стратегия pharaoh и timberborn
//https ://v3x3d.itch.io/mini-medieval
// В D:\project2026_11\Engine очень мощный терейн (небо, вода, трава, деревья, терейн через марши)
//	https://www.youtube.com/user/Lemmitbh/videos
// RPG Paper Maker 3.0
//=============================================================================
void gpu001_cube();
void gpu002_cubeSkybox();
void gpu003_spherePhong();
void gpu004_normalMap();
void gpu005_cubeMapping();
void gpu006_reflectionAndRefraction();

void Demo001();
//=============================================================================
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	//gpu001_cube();
	//gpu002_cubeSkybox();
	//gpu003_spherePhong();
	//gpu004_normalMap();
	//gpu005_cubeMapping();
	gpu006_reflectionAndRefraction();

	//Demo001();
}
//=============================================================================