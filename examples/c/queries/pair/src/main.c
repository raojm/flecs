#include <pair.h>
#include <stdio.h>

ecs_world_t *g_pstFLECSWorld = NULL;

typedef struct {
    double x;
    double y;
} Position;

ECS_STRUCT(MassEntityUniqueID, { 
    uint64_t UniqueID; 
});

ECS_STRUCT(RoomUniqueID, {
    uint16_t ID;
});

ECS_STRUCT(TableUniqueID, {
    uint16_t ID;
});

ECS_STRUCT(TestComponent1, {
    uint32_t ID1;
});

ECS_STRUCT(TestComponent2, {
    uint32_t ID1;
    uint32_t ID2;
});

void Trigger(ecs_iter_t *it) {
    // probe_system_w_ctx(it, it->ctx);

    ecs_id_t field1 = ecs_field_id(it, 1);
    ecs_id_t field2 = ecs_field_id(it, 2);


    printf("Trigger field1 id: %llu \n", field1);
    printf("Trigger field2 id: %llu \n", field2);

    for (int i = 0; i < it->count; i ++) {
        printf("entity id: %llu,  name: %s\n", it->entities[i], ecs_get_name(g_pstFLECSWorld, it->entities[i]));

        
    }
}

int main(int argc, char *argv[]) {
    g_pstFLECSWorld = ecs_init_w_args(argc, argv);

    ECS_META_COMPONENT(g_pstFLECSWorld, MassEntityUniqueID);
    ECS_META_COMPONENT(g_pstFLECSWorld, RoomUniqueID);
    ECS_META_COMPONENT(g_pstFLECSWorld, TableUniqueID);

    ECS_META_COMPONENT(g_pstFLECSWorld, TestComponent1);
    ECS_META_COMPONENT(g_pstFLECSWorld, TestComponent2);

    ECS_TAG(g_pstFLECSWorld, GameRoomTag);
    ECS_TAG(g_pstFLECSWorld, GameTableTag);

    ECS_TAG(g_pstFLECSWorld, MyTestTag1);
    ECS_TAG(g_pstFLECSWorld, MyTestTag2);

    // ecs_add(g_pstFLECSWorld, EcsWorld, RoomUniqueID);

    ecs_entity_t stObserverOnSetSystem = ecs_observer_init(g_pstFLECSWorld, &(ecs_observer_desc_t){
        .filter.terms[0].id = MyTestTag1,
        .filter.terms[1].id = EcsWildcard,
        .events = {EcsOnSet},
        .callback = Trigger,
        // .ctx = ctx_any
    });

    assert(stObserverOnSetSystem != 0);

    MassEntityUniqueID stPlayerRaojm = {.UniqueID = 100001};

    ecs_entity_t Player001 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "Player001" });
    ecs_set_ptr(g_pstFLECSWorld, Player001, MassEntityUniqueID, &stPlayerRaojm);

    ecs_set_pair(g_pstFLECSWorld, Player001, TestComponent1, ecs_id(TestComponent2), {.ID1 = 100} );
    ecs_set_pair_second(g_pstFLECSWorld, Player001, MyTestTag1, TestComponent2, {.ID1 = 200, .ID2=300} );


    ecs_entity_t stPlayerPairDataEntity = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "PlayerPairDataEntity" });
    ecs_add_pair(g_pstFLECSWorld, stPlayerPairDataEntity, EcsChildOf, Player001);

    RoomUniqueID stRoomUniqueID0001 = {.ID=0x0001};
    RoomUniqueID stRoomUniqueID0002 = {.ID=0x0002};
    RoomUniqueID stRoomUniqueID0003 = {.ID=0x0003};

    TableUniqueID stTableUniqueID0001 = {.ID=0x0001};
    TableUniqueID stTableUniqueID0002 = {.ID=0x0002};
    TableUniqueID stTableUniqueID0003 = {.ID=0x0003};
    TableUniqueID stTableUniqueID0004 = {.ID=0x0004};
    TableUniqueID stTableUniqueID0005 = {.ID=0x0005};
    TableUniqueID stTableUniqueID0006 = {.ID=0x0006};

    // ecs_new_entity(ecs, "Sun");
    //ecs_entity_init 会先add 再set name, 触发EcsOnSet时entity name会还是空
    ecs_entity_t GameRoom0001 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameRoom0001" });
    //通过.id 修改己存在的entity, 可以一次添加多个组件 节省创建多余的table类型
    ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .id = GameRoom0001, .add = { GameRoomTag,  MyTestTag1, MyTestTag2} });
    // ecs_set_id(g_pstFLECSWorld, GameRoom0001, MyTestTag2, 0, NULL);
    // ecs_add_id(g_pstFLECSWorld, GameRoom0001, MyTestTag2);
    ecs_set_ptr(g_pstFLECSWorld, GameRoom0001, RoomUniqueID, &stRoomUniqueID0001);
    ecs_entity_t GameRoom0002 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameRoom0002", .add = { GameRoomTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameRoom0002, RoomUniqueID, &stRoomUniqueID0002);
    ecs_entity_t GameRoom0003 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameRoom0003", .add = { GameRoomTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameRoom0003, RoomUniqueID, &stRoomUniqueID0003);


    ecs_entity_t GameTable0001 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameTable0001", .add = { GameTableTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameTable0001, TableUniqueID, &stTableUniqueID0001);
    ecs_add_pair(g_pstFLECSWorld, GameTable0001, EcsChildOf, GameRoom0001);
    ecs_entity_t GameTable0002 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameTable0002", .add = { GameTableTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameTable0002, TableUniqueID, &stTableUniqueID0002);
    ecs_add_pair(g_pstFLECSWorld, GameTable0002, EcsChildOf, GameRoom0002);
    ecs_entity_t GameTable0003 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameTable0003", .add = { GameTableTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameTable0003, TableUniqueID, &stTableUniqueID0003);
    ecs_add_pair(g_pstFLECSWorld, GameTable0003, EcsChildOf, GameRoom0001);
    ecs_entity_t GameTable0004 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameTable0004", .add = { GameTableTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameTable0004, TableUniqueID, &stTableUniqueID0004);
    ecs_add_pair(g_pstFLECSWorld, GameTable0004, EcsChildOf, GameRoom0002);
    ecs_entity_t GameTable0005 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameTable0005", .add = { GameTableTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameTable0005, TableUniqueID, &stTableUniqueID0005);
    ecs_add_pair(g_pstFLECSWorld, GameTable0005, EcsChildOf, GameRoom0003);
    ecs_entity_t GameTable0006 = ecs_entity_init(g_pstFLECSWorld, &(ecs_entity_desc_t){ .name = "GameTable0006", .add = { GameTableTag } });
    ecs_set_ptr(g_pstFLECSWorld, GameTable0006, TableUniqueID, &stTableUniqueID0006);

    // Create a hierarchical query to compute the global position from the
    // local position and the parent position.
    ecs_query_t *pstQuery = ecs_query(g_pstFLECSWorld, {
        .filter.terms = {
            // Read from entity's Local position
            // { .id = GameRoomTag, .src.flags = EcsParent }, 
            { .id = ecs_pair(EcsChildOf, GameRoom0001)},
            // { .id = ecs_pair(EcsChildOf, GameRoomTag)}
            // { .id = ecs_pair(EcsChildOf, GameRoom0002) }
        }
    });

    // Do the transform
    ecs_iter_t it = ecs_query_iter(g_pstFLECSWorld, pstQuery);
    while (ecs_query_next(&it)) {

        ecs_id_t field1 = ecs_field_id(&it, 1);
        auto firstPair = ecs_pair_first(g_pstFLECSWorld, field1);

        auto secondPair = ecs_pair_second(g_pstFLECSWorld, field1);

        printf("query field1 id: %llu, firstPair: %llu, second: %llu  \n", field1, firstPair, secondPair);
        // printf("query field1 id: %llu , name:%s \n", field1, ecs_get_name(g_pstFLECSWorld, field1));

        
        
        for (int i = 0; i < it.count; i ++) {
            printf("entity name: %s\n", ecs_get_name(g_pstFLECSWorld, it.entities[i]));
        }

        // const Position *p = ecs_field(&it, Position, 1);
        // Position *p_out = ecs_field(&it, Position, 2);
        // const Position *p_parent = ecs_field(&it, Position, 3);
        
        // // Inner loop, iterates entities in archetype
        // for (int i = 0; i < it.count; i ++) {
        //     printf("%s: {%f, %f}\n", ecs_get_name(g_pstFLECSWorld, it.entities[i]), p[i].x, p[i].y);
        //     p_out[i].x = p[i].x;
        //     p_out[i].y = p[i].y;
        //     if (p_parent) {
        //         p_out[i].x += p_parent->x;
        //         p_out[i].y += p_parent->y;
        //     }
        // }
    }


    ecs_progress(g_pstFLECSWorld, 0);
    ecs_progress(g_pstFLECSWorld, 0);

    // Print ecs positions
    // it = ecs_term_iter(g_pstFLECSWorld, &(ecs_term_t) {
    //     .id = ecs_pair(ecs_id(Position), World)
    // });

    // while (ecs_term_next(&it)) {
    //     Position *p = ecs_field(&it, Position, 1);
    //     for (int i = 0; i < it.count; i ++) {
    //         printf("%s: {%f, %f}\n", ecs_get_name(g_pstFLECSWorld, it.entities[i]),
    //             p[i].x, p[i].y);
    //     }
    // }

    return ecs_fini(g_pstFLECSWorld);
}
