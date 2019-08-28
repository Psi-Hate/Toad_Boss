#include <z64ovl/oot/debug.h>
#include <z64ovl/oot/types.h>
#include <z64ovl/oot/helpers.h>
 
#define TOAD_HEALTH   3  
#define ACT_ID 0x0082
#define OBJ_ID 0x0086
#define PI 3.141592654
 
/* Display Lists * * * */
     #define   DL_TOAD     0x060027B8
 
/* Animations * * * */
     #define   ANIM_TOADACTION     0x06003178
 
/* Hierarchies (Skeletons) * * * */
     #define   SKL_DEFAULT     0x06003270
 
/* Base Offset: 0x06000000 */
 
typedef struct {
    z64_actor_t actor;
    z64_actor_t dist_from_link_xz;
    z64_collider_cylinder_main_t Collision;
    z64_skelanime_t skelanime;
    uint32_t path_id;
    vec3f_t next_dest;
    int current_node;
    u32* pathlist;
    
    float last_diff;
    u8 num_nodes;
 
   
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
    , .radius = 0x0018
    , .height = 0x0028
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
    actor_set_scale(&en->actor, 0.01f);
    actor_set_height(&en->actor, 10);
    en->actor.health = TOAD_HEALTH;
    actor_init_shadow(&(en->actor).rot_2, 0, &ACTOR_SHADOW_DRAWFUNC_CIRCLE, 20.0f);
 
    // gets the list of paths and set first destination
    en->last_diff = 999999.0f;
    en->current_node = 0;
    en->pathlist = get_path_list_addr(gl);
    en->num_nodes = get_number_of_nodes_from_path(get_path_address(en->path_list, en->path_id));
    get_next_dest(en, gl);
   
   
    skelanime_init_mtx(gl, &en->skelanime, SKL_DEFAULT, ANIM_TOADACTION, 0, 0, 0);
 
    actor_collider_cylinder_init(gl, &en->Collision, &en->actor, &Collision);
 
   
}

static void play(entity_t *en, z64_global_t *gl)
{
 
    //look at player
    en->actor.rot_2.y = en->actor.rot_toward_link_y;
    
    if (en->current_node != en->num_nodes - 1)
    {
        get_next_dest(en, gl);
 
        //Calculates Arctan2 (X,Z) of two coordinates (A-B)
        //A0 = Coord A ptr | A1 = Coord B ptr | V0 = s16 rotation
        //Gets and sets the direction
        en->actor.xz_dir = math_vec3f_distance_xz(&en->actor.pos_2, &en->next_dest);  
        
        //Sets movement speed
        en->actor.xz_speed = 3;
      
        //Function to move in direction (0x32) at set velocity (0x68)
        //a0 = pointer to start address of actor instance
        external_func_8002D8E0(&en->actor);
     
        float diff = ABS(math_vec3f_distance(&en->next_dest, &en->actor.pos_2));
        
        if(diff < en->actor.xz_speed || diff > en->last_diff)
        {
            en->current_node += 1;
            en->last_diff = 999999.0f;
        }
        else
        { 
            en->last_diff = diff;
        }
    }

 
    actor_collider_cylinder_update(&en->actor, &en->Collision);
   
    if( en->actor.dist_from_link_xz < 10 )
    {
        actor_collision_check_set_at(gl, AADDR(gl,0x11e60), &en->Collision);
    }
   
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