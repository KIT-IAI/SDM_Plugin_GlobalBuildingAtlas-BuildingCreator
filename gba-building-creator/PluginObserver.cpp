#include "PluginObserver.h"
#include "gbaBuildingCreator.h"
#include "BuildingCreator.h"

PluginObserver::PluginObserver(CreateBuildingAction& createBuildingAction)
  : m_createBuildingAction(createBuildingAction)
{
}

void PluginObserver::activate(bool first)
{
}

void PluginObserver::deactivate(bool last)
{
}

void PluginObserver::selectEntity(IfcDB::ifcOid oid, bool selected)
{
}
