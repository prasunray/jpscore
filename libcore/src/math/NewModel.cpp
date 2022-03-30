#include "math/NewModel.hpp"

#include "direction/DirectionManager.hpp"

NewModel::NewModel(DirectionManager * directionManager) : OperationalModel(directionManager) {}

PedestrianUpdate NewModel::ComputeNewPosition(
    double dT,
    const Pedestrian & ped,
    const Geometry & geometry,
    const NeighborhoodSearch & neighborhoodSearch) const
{
    // TODO(Mchraibi): Implement your model update computation
    return {};
}

void NewModel::ApplyUpdate(const PedestrianUpdate & update, Pedestrian & agent) const
{
    // TODO(Mchraibi): Implement your model update application to the agent
}
