#include <LiveLogFeature.hpp>
#include <ProgressbarFeature.hpp>
#include <stdutils/utf8.hpp>
#include "BuildingCreator.h"
#include <MainFrameObserverImpl.hpp>

BuildingCreator::BuildingCreator(IfcDB::Populationi* pDB, IfcDB::utils::PopulationStates* pStates, sdm::plugin::MainFrameInterface* pInterface, sdm::plugin::LiveLogInterface* pLogger, sdm::plugin::ProgressBarInterface* pProgressBar)
  : m_pDB(pDB), m_pStates(pStates), m_pMainFrameInterface(pInterface), m_pLogger(pLogger), m_pProgressBar(pProgressBar)
{
}

BuildingCreator::~BuildingCreator()
{
}

void BuildingCreator::Build()
{
    IfcDB::IfcEntityList entities;

    IfcDB::tOidSet selectedEntities = m_pStates->getSelectedEntities();
    m_pMainFrameInterface->setSaveFileFlag(true);

    if (!selectedEntities.empty())
    {
        for (auto oid : selectedEntities)
        {
            IfcDB::IfcEntity* pEntity = m_pDB->get(oid);

            if (pEntity != nullptr)
            {
                entities.emplace_back(pEntity);
            }
        }
    }
    else
    {
        m_pDB->getAll(entities);
    }

    if (IfcDB::Feature::hasGMLClasses() == false)
    {
        IfcDB::Feature::createGMLClasses();
    }

    size_t nProcessedBuildings(0);
    IfcDB::ifcOid oidCityModel(false);

    m_pLogger->log("Comment", "Process Buildings");

    std::wstring lod(L"lod2");

    m_pProgressBar->create();
    m_pProgressBar->setProgressData(entities.size(), "Processing elements:");

    if (!entities.empty())
    {
        oidCityModel = createCityModel();
    }

    for (auto pEntity : entities)
    {
        m_pProgressBar->increment();
        m_pProgressBar->process();

        if ((pEntity->getEntityType().find(L"global3D:lod1_global") ) != std::wstring::npos)
        {
            ++nProcessedBuildings;

            createBuilding(oidCityModel, pEntity, lod);
        }
    }

    for (const auto& buildingF : buildingsMap)
    {
        m_pDB->createObjRelation(oidCityModel, buildingF->getOid(), L"core:cityObjectMember");
    }

    m_pStates->showRepresentationIdentifier(lod, true);
    m_pStates->compile();

    m_pDB->incrementSerialNo();
}

IfcDB::ifcOid BuildingCreator::createBuilding(IfcDB::ifcOid oidCityModel, IfcDB::IfcEntity* pEntity,const std::wstring &lod)
{
    IfcDB::Geometry* pGeometry = pEntity->getGeometry(L"global3D");

    if (pGeometry != nullptr)
    {
        IfcDB::Feature* pBuilding = new IfcDB::Feature(L"", L"bldg:Building", oidCityModel, true);
        pBuilding->setType(IfcDB::CITYGML_BUILDING);
        pBuilding->setState(IfcDB::STATE_INITIAL_GEOMETRY_TRANSFORMATION);
        pBuilding->setName(pEntity->getName());
        pBuilding->setStringAttribut(L"gml:name", pEntity->getName());

        m_pDB->add(pBuilding, m_pDB->getModelInfoPending());

        IfcDB::Feature* pFeature = dynamic_cast<IfcDB::Feature*>(pEntity);

        BuildingParameter buildingParameter;
        if (pFeature != nullptr)
        {
            IfcDB::UOM* pUOM(nullptr);
            double doubleAttributeValue(0.0);

            if (pFeature->getDoubleAttributWert(L"global3D:height", doubleAttributeValue, pUOM))
            {
                buildingParameter.height = doubleAttributeValue;

                if (!pUOM)
                {
                    pUOM = m_pDB->getUomList()->getUomLength();
                }

                pBuilding->setDoubleAttribut(_T("bldg:measuredHeight"), doubleAttributeValue, pUOM);
            }
        }

        if (buildingParameter.height > 0.0)
        {
            bool state = createBuidlingSolid(pEntity, buildingParameter, pBuilding);

            if (!state)
            {
                std::stringstream message;
                message << "Failed to create building solid: " << toUtf8(pBuilding->getGmlId());

                if (buildingParameter.height <= 0.0)
                {
                    message << ", incorrect building height = 0.0";
                }

                m_pLogger->log("Warning", message.str());
            }
        }
        else
        {
            std::stringstream message;
            message << "Failed to create building solid: " << toUtf8(pBuilding->getGmlId());

            if (buildingParameter.height <= 0.0)
            {
                message << ", incorrect building height = 0.0";
            }

            m_pLogger->log("Warning", message.str());
        }

        return pBuilding->getOid();
    }

    return 0;
}

bool BuildingCreator::createBuidlingSolid(IfcDB::IfcEntity* pEntity, BuildingParameter &buildingParameter, IfcDB::Feature* pBuilding)
{
    bool state(false);

    IfcDB::Profile* pProfile = nullptr;
    std::vector<IfcDB::Profile*> vProfiles;

    IfcDB::Geometry* pGeometry = pEntity->getGeometry(L"global3D");

    if (pGeometry->isA(IfcDB::MULTI_SURFACE))
    {
        IfcDB::MultiSurface* pMultiSurface = dynamic_cast<IfcDB::MultiSurface*>(pGeometry);
        for (const auto& pgeometry : pMultiSurface->getGeometries())
        {
            if (pgeometry->isA(IfcDB::FACE))
            {
                IfcDB::Face* pFace = dynamic_cast<IfcDB::Face*>(pgeometry);

                if (pFace->getOuterLoop())
                {
                    pProfile = new IfcDB::Profile();

                    pProfile->setOuterProfile(pFace->getOuterLoop()->cloneCurve());

                    for (const auto& pInnerLoop : pFace->getInnerLoops())
                    {
                        pProfile->addInnerProfile(pInnerLoop->cloneCurve());
                    }

                    vProfiles.push_back(pProfile);
                }
            }
        }
    }

    for (auto pProfile: vProfiles)
    {
        IfcDB::Matrix4* pMatrix = new IfcDB::Matrix4();
        IfcDB::Extrusion extrusion(pProfile, pMatrix, { 0.0, 0.0, 1.0 },  buildingParameter.height);
        IfcDB::Brep* brepClone{dynamic_cast<IfcDB::Brep*>(IfcDB::Geometry::toBrep(m_pDB->getPopulationContext(), &extrusion , true)) };
        IfcDB::GmlSolid* pGmlSolid = new IfcDB::GmlSolid();
        IfcDB::CompositeSurface* pCompositeSurface = new IfcDB::CompositeSurface();
        pGmlSolid->setExterior(pCompositeSurface);

        for (const auto pFace : brepClone->getFaces())
        {
            IfcDB::Face* pNewFace = dynamic_cast<IfcDB::Face*>(pFace->clone());
            pNewFace->calcFaceTransformation();

            pCompositeSurface->addGeometry(pNewFace);
        }

        std::wstring lod(L"lod2");
        std::wstring representationType(L"bldg:" + lod + L"Solid");
        IfcDB::sRepresentation representationLoD1Ridge(0, L"lod2", representationType, pGmlSolid);
        pBuilding->addRepresentation(representationLoD1Ridge);

        m_pStates->registerEntityHash(pBuilding->getOid(), representationLoD1Ridge.m_representationIdentifier,representationLoD1Ridge.m_representationType, {});
        createBoundarySurfaces(pBuilding, buildingParameter, L"lod2", true);
        buildingsMap.push_back(pBuilding);

        state = true;
    }

    return state;
}

bool BuildingCreator::createBoundarySurfaces(IfcDB::Feature* pBuilding, BuildingParameter& buildingParameter, const std::wstring& lod, bool createGeoReference/*= true*/)
{
    bool state(false);

    for (auto pGeometry : pBuilding->getGeometries())
    {
        if (pGeometry && pGeometry->getType() == IfcDB::GML_SOLID)
        {
            IfcDB::GmlSolid* pGmlSolid = dynamic_cast<IfcDB::GmlSolid*>(pBuilding->getGeometry());

            IfcDB::Geometry* pExterior = pGmlSolid->getExterior();

            if (pExterior && pExterior->getType() == IfcDB::COMPOSITE_SURFACE)
            {
                IfcDB::CompositeSurface* pCompositeSurface = dynamic_cast<IfcDB::CompositeSurface*>(pExterior);

                std::vector<IfcDB::Geometry*> references;

                for (auto pSurface : pCompositeSurface->getGeometries())
                {
                    if (pSurface->getType() == IfcDB::FACE)
                    {
                        IfcDB::Face* pFace = dynamic_cast<IfcDB::Face*>(pSurface);

                        IfcDB::Feature* pBoundarySurface = createBoundarySurface(pBuilding, buildingParameter, pFace, lod, createGeoReference);

                        // create geometry reference
                        if (createGeoReference == true)
                        {
                            IfcDB::GeometryReference* pGeoReference = new IfcDB::GeometryReference();

                            if (pFace->getGmlId().empty() == true)
                            {
                                pFace->setGmlId(_T("GML_") + IfcDB::createUuid());
                            }

                            pGeoReference->setGmlReferenceId(pFace->getGmlId());
                            pGeoReference->setGeomReferenceID(pFace->getGeomID());
                            pGeoReference->setReferencedGeometry(pFace);

                            references.emplace_back(pGeoReference);
                        }
                    }
                }

                pCompositeSurface->setGeometries(references);
            }
        }
    }

    if (pBuilding->getGeometries().empty())
    {
        return state;
    }

    return state;
}

IfcDB::Feature* BuildingCreator::createBoundarySurface(IfcDB::Feature* pBuilding, BuildingParameter& buildingParameter, IfcDB::Face* pFace, const std::wstring& lod, bool createGeoReference/*= true*/)
{
    std::wstring surfaceType;

    IfcDB::Direction normal = pFace->getFaceNormal();

    if (normal.z < -0.99)                               // ground plate, -10� -> -90�
    {
        surfaceType = _T("bldg:GroundSurface");
    }
    else if (normal.z > -0.0009 && normal.z < 0.0009)   // wall surface, -10� -> 10�
    {
        surfaceType = _T("bldg:WallSurface");
    }
    else                                                // roof surface, 10� -> 90�
    {
        surfaceType = _T("bldg:RoofSurface");
    }

    IfcDB::Feature* pBoundarySurface = m_pDB->createFeature(_T(""), surfaceType, pBuilding->getOid(), true, pBuilding->getModelInfo());

    m_pDB->createObjRelation(pBuilding->getOid(), pBoundarySurface->getOid(), _T("bldg:boundedBy"), pBuilding->getModelInfo());

    IfcDB::MultiSurface* pMultiSurface = new IfcDB::MultiSurface();

    // create geometry reference
    if (createGeoReference == true)
    {
        pMultiSurface->addGeometry(pFace);
    }
    else
    {
        IfcDB::Face* pNewFace = dynamic_cast<IfcDB::Face*>(pFace->clone());
        pNewFace->calcFaceTransformation();

        pMultiSurface->addGeometry(pNewFace);
    }

    std::wstring representationType(L"bldg:" + lod + L"MultiSurface");

    pBoundarySurface->addGeometryItem(lod, representationType, pMultiSurface);
    pBoundarySurface->setState(IfcDB::STATE_INITIAL_GEOMETRY_TRANSFORMATION);

    return pBoundarySurface;
}

IfcDB::ifcOid BuildingCreator::createCityModel()
{
    IfcDB::ModelInfo* pModelInfo = m_pDB->addModelInfo(L"", IfcDB::ModelInfo::MT_GML);

    IfcDB::Feature* pCityModel = new IfcDB::Feature(L"", L"core:CityModel", 0, true);
    pCityModel->setType(IfcDB::CITYGML_CITY_MODEL);
    pCityModel->setName(L"GlobalBuildingAtlas");

    m_pDB->add(pCityModel, pModelInfo);
    m_pDB->addGmlType(pCityModel->getOid(), IfcDB::CITYGML_2_0);

    return pCityModel->getOid();
}
