#pragma once

// TODO(mchraibi): Rename this file, class and associated enum type to represent the actual model name

#include "Geometry.hpp"
#include "direction/DirectionManager.hpp"
#include "math/OperationalModel.hpp"
#include "neighborhood/NeighborhoodSearch.hpp"
#include "pedestrian/Pedestrian.hpp"

class NewModel : public OperationalModel
{
public:
    explicit NewModel(DirectionManager * directionManager);
    ~NewModel() override = default;

    PedestrianUpdate ComputeNewPosition(
        double dT,
        const Pedestrian & ped,
        const Geometry & geometry,
        const NeighborhoodSearch & neighborhoodSearch) const override;

    void ApplyUpdate(const PedestrianUpdate & update, Pedestrian & agent) const override;
};
