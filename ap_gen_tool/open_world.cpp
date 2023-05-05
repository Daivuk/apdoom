#include <onut/onut.h>
#include <onut/Settings.h>
#include <onut/Renderer.h>
#include <onut/PrimitiveBatch.h>
#include <onut/Window.h>
#include <onut/Input.h>

#include <imgui/imgui.h>

#include <vector>

#include "maps.h"
#include "generate.h"


enum class state_t
{
    idle,
    panning,
    gen,
    gen_panning
};


struct map_state_t
{
    Vector2 pos;
    float angle = 0.0f;
};


struct map_view_t
{
    Vector2 cam_pos;
    float cam_zoom = 0.25f;
};


struct map_history_t
{
    std::vector<map_state_t> history;
    int history_point = 0;
};


static int active_ep = 0;
static int active_map = 0;
static map_state_t map_states[EP_COUNT][MAP_COUNT];
static map_view_t map_views[EP_COUNT][MAP_COUNT];
static map_history_t map_histories[EP_COUNT][MAP_COUNT];
static state_t state = state_t::idle;
static Vector2 mouse_pos;
static Vector2 mouse_pos_on_down;
static Vector2 cam_pos_on_down;
static map_state_t map_state;
static map_view_t* map_view = nullptr;
static map_history_t* map_history = nullptr;


void update_window_title()
{
    oWindow->setCaption(level_names[active_ep][active_map]);
}


void select_map(int ep, int map)
{
    active_ep = ep;
    active_map = map;
    map_state = map_states[active_ep][active_map];
    map_view = &map_views[active_ep][active_map];
    map_history = &map_histories[active_ep][active_map];
    update_window_title();
}


// Undo/Redo shit
void push_undo()
{
    if (map_history->history_point < (int)map_histories[active_ep][active_map].history.size() - 1)
        map_history->history.erase(map_history->history.begin() + (map_history->history_point + 1), map_history->history.end());
    map_history->history.push_back(map_state);
    map_history->history_point = (int)map_history->history.size() - 1;
}


void undo()
{
    if (map_history->history_point > 0)
    {
        map_history->history_point--;
        map_state = map_history->history[map_history->history_point];
    }
}


void redo()
{
    if (map_history->history_point < (int)map_history->history.size() - 1)
    {
        map_history->history_point++;
        map_state = map_history->history[map_history->history_point];
    }
}


void initSettings()
{
    oSettings->setGameName("APDOOM Gen Tool");
    oSettings->setResolution({ 1600, 900 });
    oSettings->setIsResizableWindow(true);
    oSettings->setIsFixedStep(false);
    oSettings->setShowFPS(false);
    oSettings->setAntiAliasing(true);
    oSettings->setShowOnScreenLog(false);
}


void init()
{
    init_maps();
    select_map(0, 0);

    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        for (int map = 0; map < MAP_COUNT; ++map)
        {
            auto k = ep * 9 + map;
            map_states[ep][map].pos = Vector2(
                -16384 + 3000 + ((k % 5) * 6000),
                -16384 + 3000 + ((k / 5) * 6000)
            );
        }
    }
}


void shutdown()
{
}


void update_shortcuts()
{
    auto ctrl = OInputPressed(OKeyLeftControl);
    auto shift = OInputPressed(OKeyLeftShift);
    auto alt = OInputPressed(OKeyLeftAlt);

    if (ctrl && !shift && !alt && OInputJustPressed(OKeyZ)) undo();
    if (ctrl && shift && !alt && OInputJustPressed(OKeyZ)) redo();
}


void update()
{
    // Update mouse pos in world
    auto cam_matrix = Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom);
    auto inv_cam_matrix = cam_matrix.Invert();
    mouse_pos = Vector2::Transform(OGetMousePos(), inv_cam_matrix);

    // Update shortcuts
    switch (state)
    {
        case state_t::idle:
        {
            update_shortcuts();
            if (OInputJustPressed(OMouse3))
            {
                state = state_t::panning;
                mouse_pos_on_down = OGetMousePos();
                cam_pos_on_down = map_view->cam_pos;
            }
            else if (oInput->getStateValue(OMouseZ) > 0.0f)
            {
                map_view->cam_zoom *= 1.2f;
            }
            else if (oInput->getStateValue(OMouseZ) < 0.0f)
            {
                map_view->cam_zoom /= 1.2f;
            }
            else if (OInputJustPressed(OKeyTab))
            {
                state = state_t::gen;
            }
            break;
        }
        case state_t::panning:
        {
            auto diff = OGetMousePos() - mouse_pos_on_down;
            map_view->cam_pos = cam_pos_on_down - diff / map_view->cam_zoom;
            if (OInputJustReleased(OMouse3))
                state = state_t::idle;
            break;
        }
        case state_t::gen:
        {
            if (OInputJustPressed(OMouse3))
            {
                state = state_t::gen_panning;
                mouse_pos_on_down = OGetMousePos();
                cam_pos_on_down = map_view->cam_pos;
            }
            else if (oInput->getStateValue(OMouseZ) > 0.0f)
            {
                map_view->cam_zoom *= 1.2f;
            }
            else if (oInput->getStateValue(OMouseZ) < 0.0f)
            {
                map_view->cam_zoom /= 1.2f;
            }
            else if (OInputJustPressed(OKeyTab))
            {
                state = state_t::idle;
            }
            break;
        }
        case state_t::gen_panning:
        {
            auto diff = OGetMousePos() - mouse_pos_on_down;
            map_view->cam_pos = cam_pos_on_down - diff / map_view->cam_zoom;
            if (OInputJustReleased(OMouse3))
                state = state_t::gen;
            break;
        }
    }
}


void draw_guides()
{
    auto pb = oPrimitiveBatch.get();

    pb->draw(Vector2(0, -16384), Color(0.15f)); pb->draw(Vector2(0, 16384), Color(0.15f));
    pb->draw(Vector2(-16384, 0), Color(0.15f)); pb->draw(Vector2(16384, 0), Color(0.15f));

    pb->draw(Vector2(-16384, -16384), Color(1, 0, 0)); pb->draw(Vector2(-16384, 16384), Color(1, 0, 0));
    pb->draw(Vector2(-16384, 16384), Color(1, 0, 0)); pb->draw(Vector2(16384, 16384), Color(1, 0, 0));
    pb->draw(Vector2(16384, 16384), Color(1, 0, 0)); pb->draw(Vector2(16384, -16384), Color(1, 0, 0));
    pb->draw(Vector2(16384, -16384), Color(1, 0, 0)); pb->draw(Vector2(-16384, -16384), Color(1, 0, 0));
}


void draw_level(int ep, int lvl, const Vector2& pos, float angle)
{
    Color bound_color(1.0f);
    Color step_color(0.35f);
    Color bb_color(1, 1, 0, 1);

    auto pb = oPrimitiveBatch.get();
    auto map = &maps[ep][lvl];

    Vector2 mid((map->bb[2] + map->bb[0]) / 2, -(map->bb[3] + map->bb[1]) / 2);

    pb->begin(OPrimitiveLineList, nullptr, 
              Matrix::CreateRotationZ(angle) *
              Matrix::CreateTranslation(pos - mid) *
              Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom)
    );

    // Bounding box
    pb->draw(Vector2(map->bb[0], -map->bb[1]), bb_color); pb->draw(Vector2(map->bb[0], -map->bb[3]), bb_color);
    pb->draw(Vector2(map->bb[0], -map->bb[3]), bb_color); pb->draw(Vector2(map->bb[2], -map->bb[3]), bb_color);
    pb->draw(Vector2(map->bb[2], -map->bb[3]), bb_color); pb->draw(Vector2(map->bb[2], -map->bb[1]), bb_color);
    pb->draw(Vector2(map->bb[2], -map->bb[1]), bb_color); pb->draw(Vector2(map->bb[0], -map->bb[1]), bb_color);

    // Geometry
    for (const auto& line : map->linedefs)
    {
        Color color = bound_color;
        if (line.back_sidedef != -1) color = step_color;
        
        pb->draw(Vector2(map->vertexes[line.start_vertex].x, -map->vertexes[line.start_vertex].y), color);
        pb->draw(Vector2(map->vertexes[line.end_vertex].x, -map->vertexes[line.end_vertex].y), color);
    }

    pb->end();
}


void render()
{
    oRenderer->clear(Color::Black);
    auto pb = oPrimitiveBatch.get();

    pb->begin(OPrimitiveLineList, nullptr, Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom));
    draw_guides();
    pb->end();

    switch (state)
    {
        case state_t::gen:
        case state_t::gen_panning:
        {
            for (int ep = 0; ep < EP_COUNT; ++ep)
            {
                for (int map = 0; map < MAP_COUNT; ++map)
                {
                    draw_level(ep, map, map_states[ep][map].pos, map_states[ep][map].angle);
                }
            }
            break;
        }
        default:
        {
            draw_level(active_ep, active_map, {0, 0}, 0);
            break;
        }
    }
}


void renderUI()
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Generate")) generate();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) OQuit();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Maps"))
    {
        for (int ep = 0; ep < EP_COUNT; ++ep)
        {
            for (int map = 0; map < MAP_COUNT; ++map)
            {
                if (ImGui::MenuItem(level_names[ep][map]))
                {
                    select_map(ep, map);
                }
            }
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}


void postRender()
{
}
