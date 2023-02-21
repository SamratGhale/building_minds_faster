#ifndef SIM_REGION

struct SimRegion{
    U32 max_count;
    U32 entity_count;
    Entity* entities;
};

#define SIM_REGION
#endif