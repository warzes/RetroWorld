#include "stdafx.h"
#include "map_man.hpp"
//=============================================================================
ed::MapMan::TileAction::TileAction(size_t i, size_t j, size_t k,
	TileGrid prevState, TileGrid newState)
	: _i(i), _j(j), _k(k)
	, _prevState(std::move(prevState))
	, _newState(std::move(newState))
{}
//=============================================================================
void ed::MapMan::TileAction::Do(MapMan& map) const
{
	map._tileGrid.CopyTiles(
		static_cast<int>(_i), static_cast<int>(_j), static_cast<int>(_k),
		_newState, false);
}
//=============================================================================
void ed::MapMan::TileAction::Undo(MapMan& map) const
{
	map._tileGrid.CopyTiles(
		static_cast<int>(_i), static_cast<int>(_j), static_cast<int>(_k),
		_prevState, false);
}
//=============================================================================
ed::MapMan::EntAction::EntAction(size_t i, size_t j, size_t k,
	bool overwrite, bool removed, Ent oldEnt, Ent newEnt)
	: _i(i), _j(j), _k(k)
	, _overwrite(overwrite), _removed(removed)
	, _oldEnt(std::move(oldEnt)), _newEnt(std::move(newEnt))
{}
//=============================================================================
void ed::MapMan::EntAction::Do(MapMan& map) const
{
	if (_removed)
		map._entGrid.RemoveEnt(static_cast<int>(_i), static_cast<int>(_j), static_cast<int>(_k));
	else
		map._entGrid.AddEnt(static_cast<int>(_i), static_cast<int>(_j), static_cast<int>(_k), _newEnt);
}
//=============================================================================
void ed::MapMan::EntAction::Undo(MapMan& map) const
{
	if (_overwrite)
		map._entGrid.AddEnt(static_cast<int>(_i), static_cast<int>(_j), static_cast<int>(_k), _oldEnt);
	else
		map._entGrid.RemoveEnt(static_cast<int>(_i), static_cast<int>(_j), static_cast<int>(_k));
}
//=============================================================================