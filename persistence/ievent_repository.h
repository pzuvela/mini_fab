#pragma once

#include <cstdint>
#include <vector>

#include "equipment/equipment_event.h"

class IEventRepository
{
public:
    virtual std::uint64_t insert(const EquipmentEvent& event) = 0;

    virtual std::vector<EquipmentEvent> getAll() const = 0;

    virtual ~IEventRepository() = default;
};

