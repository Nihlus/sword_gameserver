#ifndef GAME_MODES_HPP_INCLUDED
#define GAME_MODES_HPP_INCLUDED

#include <vector>
#include <string>
#include <vec/vec.hpp>

namespace game_mode
{
    enum game_mode : int32_t
    {
        FIRST_TO_X,
        FFA,
        ///the man with the golden sword, and in fact outfit entirely
        COUNT
    };

    static std::vector<std::string> names
    {
        "Team Deathmatch",
        "Error"
    };
}

#define TEAM_NUMS 16

typedef game_mode::game_mode game_mode_t;

namespace game_data
{
    const float kills_to_win = 40;
    const float round_default = 3.5 * 60; ///10 seconds for testing
    const float game_over_time = 6; ///seconds
}

struct session_boundaries
{
    int32_t max_kills = game_data::kills_to_win;
    float max_time_ms = game_data::round_default * 1000.f;
};

struct session_state
{
    int32_t team_killed[TEAM_NUMS] = {0}; ///how many died on this team
    int32_t team_kills[TEAM_NUMS] = {0}; ///how many this team has killed

    float time_elapsed = 0; ///if i make you a float, i can just pipe these structures directly

    ///implement gamemode functions here, with session_boundaries

    bool game_over(const session_boundaries& bounds);
    std::string get_game_over_string(const session_boundaries& bounds);
    std::string get_current_game_state_string(const session_boundaries& bounds);
};

namespace map_namespace
{
    enum spawn_defs : int
    {
        R = -1 << 16,
        B = -1 << 17//,
        //G = -1 << 18  //GOLD
    };

    static std::vector<spawn_defs> team_list
    {
        R,
        B//,
        //G
    };

    enum map_cube_t
    {
        BOTTOM = 0,
        TOP = 1,
        FRONT = 2,
        LEFT = 3,
        RIGHT = 4,
        BEHIND = 5,
    };

    ///x = either axis here

    ///eg if my axis goes 0 -> x, and x = 0 @ x = x and x -> x on the second plane like the first
    ///we'd be a no
    enum axis_are_flipped
    {
        NO,
        YES, ///x -> y
        NEG, ///x -> -x
        YES_NEG, ///x -> -y
    };

    static std::vector<vec3f> map_cube_rotations =
    {
        {0,0,0},
        {M_PI,0,0},
        {M_PI/2,0,0},
        {0,0,M_PI/2},
        {0,0,-M_PI/2},
        {-M_PI/2,0,0},
    };

    ///from 0 -> map_cube_t
    ///+y, -y, +x, -x
    static std::vector<std::vector<map_cube_t>> connection_map
    {
        ///so, bottom +y connects to front, -y connects to behind etc
        {FRONT, BEHIND, RIGHT, LEFT}, ///BOTTOM
        {BEHIND, FRONT, RIGHT, LEFT}, ///TOP
        {TOP, BOTTOM, RIGHT, LEFT}, ///FRONT
        {FRONT, BEHIND, BOTTOM, TOP}, ///LEFT
        {FRONT, BEHIND, TOP, BOTTOM}, ///RIGHT
        {TOP, BOTTOM, RIGHT, LEFT}, ///BEHIND
    };

    ///to be fair, i could  have just defined the vecs in 2d space and probably worked it out from there
    ///as a net, or something
    ///this requires less figuring though, although its not very elegant
    static std::vector<std::vector<axis_are_flipped>> axis_map
    {
        {NO, NO, NO, NO},
        {NO, NO, NEG, NEG},
        {NO, NO, YES_NEG, YES},
        {YES, YES_NEG, NO, NEG},
        {YES_NEG, YES, NEG, NO},
        {NO, NO, YES, YES_NEG},
    };

    ///rotations are defined from the reference plane of the bottom

    /*static std::vector<int>
    map_one
    {
        1,1,1,1,1,1,1,1,1,1,1,
        1,0,0,R,R,R,R,R,0,0,1,
        1,0,0,0,0,0,0,0,0,0,1,
        1,0,0,1,1,1,1,1,0,0,1,
        1,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,1,0,1,0,0,0,1,
        1,0,0,0,1,0,1,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,1,
        1,0,0,1,1,1,1,1,0,0,1,
        1,0,0,0,0,0,0,0,0,0,1,
        1,0,0,B,B,B,B,B,0,0,1,
        1,1,1,1,1,1,1,1,1,1,1,
    };*/

    static std::vector<int>
    map_one
    {
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,0,0,0,0,R,R,R,R,R,R,R,R,R,R,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,R,R,R,R,R,R,R,R,R,R,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,2,1,2,1,2,1,2,1,2,1,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,1,2,1,2,1,2,1,2,1,2,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,2,1,0,0,1,2,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,1,0,0,0,2,1,0,0,1,2,0,0,0,1,0,0,1,1,1,
        1,1,1,0,0,1,0,0,0,2,1,0,0,1,2,0,0,0,1,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,2,1,0,0,1,2,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,2,1,2,1,2,1,2,1,2,1,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,1,2,1,2,1,2,1,2,1,2,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,B,B,B,B,B,B,B,B,B,B,0,0,0,0,1,1,1,
        1,1,1,0,0,0,0,B,B,B,B,B,B,B,B,B,B,0,0,0,0,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    };

    static std::vector<std::vector<int>> map_cube_one
    {
        map_one,
        map_one,
        map_one,
        map_one,
        map_one,
        map_one
    };
}


namespace game_map
{
    //static float scale = 1000.f;
    static float scale = 500.f;
    //static float floor_const =
    #define FLOOR_CONST (bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3)

    static
    std::vector<std::vector<int>> map_list =
    {
        map_namespace::map_one
    };

    static std::vector<vec2i> map_dims =
    {
        {12*2, 12*2}
    };

    static vec2i world_to_point(vec2f world_pos, vec2f dim)
    {
        vec2f centre = dim/2.f;

        vec2f map_scale = world_pos / game_map::scale;

        map_scale = map_scale - 0.5f;

        map_scale = round(map_scale + centre);

        vec2i imap = {(int)map_scale.v[0], (int)map_scale.v[1]};

        imap = clamp(imap, (vec2i){0,0}, (vec2i){(int)dim.v[0], (int)dim.v[1]} - 1);

        return imap;
    }

    static vec2f point_to_world(vec2f local_pos, vec2f dim)
    {
        vec2f w2d = local_pos - dim/2.f;

        vec2f world_pos = w2d + (vec2f){0.5f, 0.5f};

        return world_pos * scale;
    }

    ///universally shared between client and server
    static std::vector<vec2f> get_spawn_positions(map_namespace::spawn_defs team, int map_id)
    {
        std::vector<vec2f> ret;

        const std::vector<int>& my_map = map_list[map_id];

        vec2i dim = map_dims[map_id];

        for(int y=0; y<dim.v[1]; y++)
        {
            for(int x=0; x<dim.v[0]; x++)
            {
                int point = my_map[y*dim.v[0] + x];

                if(point != (int)team)
                    continue;

                vec2f wpos = point_to_world({x, y}, {dim.v[0], dim.v[1]});

                ret.push_back(wpos);
            }
        }

        return ret;
    }
}

#endif // GAME_MODES_HPP_INCLUDED
