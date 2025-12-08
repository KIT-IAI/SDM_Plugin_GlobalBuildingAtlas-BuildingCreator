#pragma once

struct BuildingParameter{

    double height = 0;
    double elevation = 0;
    std::wstring roof_shape = L"flat";
    std::unordered_map<std::wstring, std::wstring> MapRoofShape2RoofTypeCodelist{ {L"gabled",   L"1030"},
                                                                                  {L"skillion", L"1020"},
                                                                                  {L"flat",     L"1000"},
                                                                                  {L"hipped",   L"1040"} };
};