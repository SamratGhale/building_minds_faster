
function SimRegion* begin_sim(WorldPosition region_center, Rec2 region_bounds){
}

function void end_sim(SimRegion* region){
    Entity* entity = region->entities;
    for (i = 0; i <region->entity_count; i += 1, ++entity){
        //TODO(samrat): Store entity here
        //Literally never ever ever translate between hgih and low entity except here
    }
}