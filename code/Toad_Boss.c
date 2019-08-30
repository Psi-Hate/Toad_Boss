#include <z64ovl/oot/debug.h>
#include<z64ovl/oot/helpers.h>
#include <z64ovl/oot/sfx.h>


#define TOAD_HEALTH   3  
#define ACT_ID 0x0082
#define OBJ_ID 0x0086
#define GLOBAL_SCENE_FRAME 0x802120BC

/* Display Lists * * * */
     #define   DL_TOAD     0x060027B8


typedef struct {
    z64_actor_t actor;
    z64_actor_t dist_from_link_xz;
    z64_collider_cylinder_main_t Collision;
    z64_skelanime_t skelanime;
    uint32_t path_id;
    vec3f_t next_dest;
    int count;
    int current_node;
    u32* pathlist;
    float last_diff;
    u8 num_nodes;
    bool touching_ground;
    vec3f_t curPos;
    vec3f_t destPos;
    float jumpTime;
    bool isJumping;
    int jumpStance;
    int fightStance;
    int stance;
    int randnum;


} entity_t;
 
const z64_collider_cylinder_init_t Collision =
{
    .body = {
          .unk_0x14 = 0x0A
        , .collider_flags = 0x00
        , .collide_flags = 0x11
        , .mask_a = 0x39
        , .mask_b = 0x10
        , .type = 0x01
        , .body_flags = 0x00
        , .toucher_mask = 0xFFCFFFFF
        , .bumper_effect = 0x00
        , .toucher_damage = 0x04
        , .bumper_mask = 0xFFCFFFFF
        , .toucher_flags = 0x00
        , .bumper_flags = 0x01
        , .body_flags_2 = 0x01
    }
    , .radius = 0x050
    , .height = 0x0050
    , .y_shift = 0
    , .position = {
          .x = 0
        , .y = 0
        , .z = 0
    }
};

void get_next_dest(entity_t *en, z64_global_t *gl)
{
    //sets the node data within a path to dest_pos
    vec3s_t *dest_pos = (vec3s_t*)get_node_data_from_path(en->pathlist , en->path_id, en->current_node);
    //converts vec3s data type to vec3f
    math_vec3f_from_vec3s(&en->next_dest, dest_pos);
} 
 
/*** functions ***/
static void init(entity_t *en, z64_global_t *gl)
{
    //adds  gravity
	en->actor.pos_2.y -=10;

    en->isJumping = false;
    actor_collider_cylinder_init(gl, &en->Collision, &en->actor, &Collision);
    actor_set_scale(&en->actor, 0.01f);
    actor_set_height(&en->actor, 10);
    
    en->actor.health = TOAD_HEALTH;
    actor_init_shadow(&(en->actor).rot_2, 0, &ACTOR_SHADOW_DRAWFUNC_CIRCLE, 50.0f);
 
    en->actor.gravity = -0.5f;

    // gets the list of paths and set first destination
    en->last_diff = 999999.0f;
    en->current_node = 0;
    en->path_id = 0;
    en->pathlist = get_path_list_addr(gl);
    en->num_nodes = get_number_of_nodes_from_path(get_path_address(en->pathlist, en->path_id));
    get_next_dest(en, gl);
    en->actor.mass = 0xF0;
    en->count = 0;
    en->stance = en->jumpStance;
    //skelanime_init_mtx(gl, &en->skelanime, SKL_DEFAULT, ANIM_TOADACTION, 0, 0, 0);

}

int check_stance_jump(entity_t *en, z64_global_t *gl)
{
    if(en->count >= 80)
    {
        en->count = 0;
    }
    en->count += 1;
}

int check_stance_fight(entity_t *en, z64_global_t *gl)
{
    if(en->count >= 80)
    {
        en->stance = en->fightStance;
        en->count = 0;
    }
    en->count += 1;
}

void HandleJump(entity_t *en, z64_global_t *gl) 
{

    if (en->isJumping) {
        en->jumpTime += (1.0f / 20.0f);
        float delta = en->jumpTime;

        if (delta < 1.0f)
        {
            // Parabola code!
            float time = delta;
            float ptime = time * 2 - 1;

            // Multiply for temp vectors
            vec3f_t tdest = en->destPos;
            tdest.x = (tdest.x - en->curPos.x) * time;
            tdest.y = (tdest.y - en->curPos.y) * time;
            tdest.z = (tdest.z - en->curPos.z) * time;
            vec3f_t tcur = en->curPos;
            tcur.x += tdest.x;
            tcur.y += tdest.y;
            tcur.z += tdest.z;

            tcur.y += (-ptime * ptime + 1) * 300;
            en->actor.pos_2 = tcur;
        }
        else if (delta >= 1.0f)
        {
            // Reset timer
            en->jumpTime = 0.0f;

            // Prevent desync.
            en->actor.pos_2 = en->destPos;

            en->isJumping = false;
        }
    }
    else 
    {
        if((en->actor.bgcheck_flags & 0xB) || (en->Collision.body.flags_2 & 2)) 
        {
            en->actor.xz_speed = 100;
            get_next_dest(en, gl);
            en->actor.rot_2.y = external_func_80078068(&en->actor.pos_2, &en->next_dest);
            en->curPos = en->actor.pos_2;
            en->destPos = en->next_dest;
            en->isJumping = true;
            sound_play_actor(&en->actor, NA_SE_IT_HAND_CLAP);
            
        }
        else 
        {
            en->actor.xz_speed = 0;
            en->actor.gravity = -5;
            en->actor.pos_2.y = 5;
            external_func_8002D8E0(&en->actor);
        }
    }
}

void HandleFight(entity_t *en, z64_global_t *gl) 
{
    check_stance_fight(en, gl);
    sound_play_actor(&en->actor, NA_SE_EV_EXPLOSION);
}


static void play(entity_t *en, z64_global_t *gl)
{

    actor_collider_cylinder_update(&en->actor, &en->Collision);
	external_func_8002E4B4(gl, &en->actor, 50.0f, 10.0f, 100.0f, 5); //extern void external_func_8002E4B4(z64_global_t *global, z64_actor_t *actor, f32 below, f32 radius, f32 above, u32 flags);

    if (en->stance == en->jumpStance) 
    {
        HandleJump(en, gl);
        
    }
    if (en->stance == en->fightStance)
    {
        HandleFight(en, gl);
    }
    
    //HandleJump(en,gl);

    float diff = ABS(math_vec3f_distance(&en->next_dest, &en->actor.pos_2));
        

    if(diff < en->actor.xz_speed)
    {
        en->current_node += 1;
    }
    if (en->current_node == en->num_nodes - 1)
    {
        en->current_node = 0;
    }

    //external_func_8002D8E0(&en->actor);
    actor_collider_cylinder_update(&en->actor, &en->Collision);
    actor_collision_check_set_ac(gl,AADDR(gl, 0x011E60), &en->Collision);
	actor_collision_check_set_at(gl,AADDR(gl, 0x011E60), &en->Collision);
    actor_collision_check_set_ot(gl, (u32*)(AADDR(gl,0x11e60)), &en->Collision);
}
 
static void draw(entity_t *en, z64_global_t *gl)
{
    /*actor_anime_frame_update_mtx(&en->skelanime);
    actor_set_height(&en->actor, 50);
    skelanime_draw_mtx(
        gl,
        en->skelanime.limb_index,
        en->skelanime.unk5,
        en->skelanime.dlist_count,
        0, 0,
        &en->actor
    );*/
    draw_dlist_opa(gl, DL_TOAD);
    actor_set_height(&en->actor, 22);
 
}

static void dest(entity_t *en, z64_global_t *gl)
{
}

const z64_actor_init_t init_vars = {
    .number = ACT_ID,
    .type = 0x05, //Enemy
    .room = 0x00,
    .flags = 0x00000010,
    .object = OBJ_ID,
    .padding = 0x0000,
    .instance_size = sizeof(entity_t),
    .init = init,
    .dest = dest,
    .main = play,
    .draw = draw
};