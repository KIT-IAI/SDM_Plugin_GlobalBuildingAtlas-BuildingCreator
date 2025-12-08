#pragma once

#include <ifcdb/IfcDbInclude.h>
#include <ifcdb/utils/PopulationObserver.hpp>
#include <unordered_map>
#include "BuildingParameter.h"

namespace sdm
{
	namespace plugin
	{
		class LiveLogInterface;
        class MainFrameInterface;
        class ProgressBarInterface;
	}
}

class BuildingCreator
{
  public:
    BuildingCreator(IfcDB::Populationi* pDB, IfcDB::utils::PopulationStates* pStates, sdm::plugin::MainFrameInterface* pInterface, sdm::plugin::LiveLogInterface* pLogger, sdm::plugin::ProgressBarInterface* pProgressBar);
    ~BuildingCreator();

    void Build();
    void setLogger(sdm::plugin::LiveLogInterface* pLogger) { m_pLogger = pLogger; }

    IfcDB::ifcOid createBuilding(IfcDB::ifcOid oidCityModel, IfcDB::IfcEntity* pEntity, const std::wstring& lod);
    IfcDB::ifcOid createCityModel();

    bool createBuidlingSolid(IfcDB::IfcEntity* pEntity, BuildingParameter& buildingParameter, IfcDB::Feature* pBuilding);
    bool createBoundarySurfaces(IfcDB::Feature* pBuilding, BuildingParameter& buildingParameter, const std::wstring& lod, bool createGeoReference = true);
    IfcDB::Feature* createBoundarySurface(IfcDB::Feature* pBuilding, BuildingParameter& buildingParameter, IfcDB::Face* pFace, const std::wstring& lod, bool createGeoReference = true);

    std::vector<IfcDB::Feature*> buildingsMap;

  private:
    IfcDB::Populationi* m_pDB = nullptr;
    IfcDB::utils::PopulationStates* m_pStates = nullptr;
    sdm::plugin::LiveLogInterface* m_pLogger = nullptr;
    sdm::plugin::MainFrameInterface* m_pMainFrameInterface = nullptr;
    sdm::plugin::ProgressBarInterface* m_pProgressBar = nullptr;
};
