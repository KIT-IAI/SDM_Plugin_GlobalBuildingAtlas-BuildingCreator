#include <iostream>
#include <string>
#include <vector>
#include <stdutils/executable.hpp>

#include "gbaBuildingCreator.h"
#include "BuildingCreator.h"

#include <ifcdb/IFCDBInterfaceVersion.hpp>
#include "PluginInterfaceVersion.hpp"
#include "ComponentRegistry.hpp"

using namespace std;
using namespace sdm::plugin;

IMPLEMENT_PLUGIN(gbaBuildingCreator);

gbaAction::gbaAction()
  : ActionFeatureHelper("Global Building Atlas Building Creator")
{
}

gbaAction::~gbaAction()
{
}

void gbaAction::execute() const
{
    BuildingCreator buildingCreator(m_pDB, m_pStates, m_pMainFrameInterface, m_pLogger, m_pProgressBar);
    buildingCreator.Build();
}

bool gbaAction::isActive() const
{
  if (m_pDB && !m_isDialogOpen && m_pDB->hasModelInfo(IfcDB::ModelInfo::MT_GLOBALBUILDINGATLAS))
  {
    return true;
  }

  return false;
}


gbaBuildingCreator::gbaBuildingCreator()
{
  m_mainframeObserver.attach([this](MainFrameInterface* pInterface) { m_gbaAction.setInterface(pInterface); });
  m_liveLogObserver.attach([this](LiveLogInterface* liveLogInterface) { m_gbaAction.setLogger(liveLogInterface); });
  m_progressBarFeature.attach([this](ProgressBarInterface* pProgressBar) { m_gbaAction.setProgressBar(pProgressBar); });
  m_documentObserver.attach([this](IfcDB::Populationi* pDB) { setDB(pDB); });
  m_documentObserver.attach([this](IfcDB::utils::PopulationSubject* pStates) { setStates(dynamic_cast<IfcDB::utils::PopulationStates*>(pStates)); });
}

sdm::plugin::Version gbaBuildingCreator::getInterfaceVersion() const
{
  return sdm::plugin::Version();
}

sdm::plugin::PluginInfo gbaBuildingCreator::getInfo() const
{
  PluginInfo info;
  info.name = "GlobalBuildingAtlas BuildingCreator";
  info.description = "";
  info.version     = { 0, 1 };

  return info;
}

std::vector<Feature*> gbaBuildingCreator::getFeatures() const
{
  std::vector<Feature*> features;

  features.emplace_back(const_cast<gbaAction*>(&m_gbaAction));
  features.emplace_back(const_cast<MainFrameObserverImpl*>(&m_mainframeObserver));
  features.emplace_back(const_cast<LiveLogObserver*>(&m_liveLogObserver));
  features.emplace_back(const_cast<ProgressBarFeature*>(&m_progressBarFeature));
  features.emplace_back(const_cast<DocumentObserverImpl*>(&m_documentObserver));

  return features;
}

ComponentInfo gbaBuildingCreator::getComponentInfo(const RequiredComponent& requiredComponent) const
{
  ComponentRegistry availableComponents;
  availableComponents.addAvailable(IFCDB_INTERFACE_COMPONENT_NAME, IFCDB_INTERFACE_COMPONENT_VERSION, IFCDB_INTERFACE_COMPONENT_HINT, std::atoi(IFCDB_INTERFACE_COMPONENT_DATE.data()));
  availableComponents.addAvailable(PLUGIN_INTERFACE_COMPONENT_NAME, PLUGIN_INTERFACE_COMPONENT_VERSION, PLUGIN_INTERFACE_COMPONENT_HINT, std::atoi(PLUGIN_INTERFACE_COMPONENT_DATE.data()));

  return availableComponents.getInfo(requiredComponent);
}

const sdm::plugin::InitializationState& gbaBuildingCreator::getInitializationState() const
{
  return m_initState;
}

void gbaBuildingCreator::setDB(IfcDB::Populationi* pDB)
{
  IfcDB::assignGlobalStates(pDB);
  m_gbaAction.setDB(pDB);
}

void gbaBuildingCreator::setStates(IfcDB::utils::PopulationStates* pStates)
{
  m_gbaAction.setStates(pStates);
}
