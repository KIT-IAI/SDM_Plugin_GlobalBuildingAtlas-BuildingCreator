#pragma once

#include <ifcdb/utils/PopulationObserver.hpp>

class MyClass;
class CreateBuildingAction;

class PluginObserver final : public IfcDB::utils::PopulationObserver
{
  public:
    explicit PluginObserver(CreateBuildingAction& createBuildingAction);
    ~PluginObserver() override = default;

    void activate(bool first) override;
    void deactivate(bool last) override;
    void selectEntity(IfcDB::ifcOid oid, bool selected) override;

  private:
    CreateBuildingAction& m_createBuildingAction;
};