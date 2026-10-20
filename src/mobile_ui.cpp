#include "mobile_ui.h"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "game.h"
#include "character.h"
#include "avatar.h"
#include "options.h"
#include "output.h"
#include "translations.h"
#include "input_context.h"
#include "sdl_font.h"
#include "item.h"
#include "item_location.h"
#include "path_info.h"
#include "filesystem.h"
#include "cata_utility.h"
#include "json.h"
#include <set>
#include <fstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cata_mobile
{

mobile_ui_system &mobile_ui_system::get()
{
    static mobile_ui_system instance;
    return instance;
}

mobile_ui_system::mobile_ui_system()
{
    init();
}

void mobile_ui_system::init()
{
    if( initialized ) {
        return;
    }
    setup_default_buttons();
    calculate_geometry( screen_width, screen_height );
    load_all_schemes();
    initialized = true;
}

void mobile_ui_system::setup_default_buttons()
{
    fixed_buttons.clear();
    right_context_slots.clear();
    left_context_slots.clear();

    // 1. PPSSPP Style Face Buttons (Right cluster diamond)
    touch_button btn_south; // Confirm / Autoattack
    btn_south.id = button_id::FACE_SOUTH;
    btn_south.style = button_visual_style::CIRCLE;
    btn_south.game_action = ACTION_AUTOATTACK;
    btn_south.game_fallback_key = '\t';
    btn_south.menu_fallback_key = '\n';
    btn_south.default_symbol = "X";
    btn_south.description = _( "Ataque / Confirmar" );
    btn_south.border_r = 70; btn_south.border_g = 140; btn_south.border_b = 255;
    fixed_buttons.push_back( btn_south );

    touch_button btn_east; // Cancel / Look
    btn_east.id = button_id::FACE_EAST;
    btn_east.style = button_visual_style::CIRCLE;
    btn_east.game_action = ACTION_LOOK;
    btn_east.game_fallback_key = 'x';
    btn_east.menu_fallback_key = KEY_ESCAPE;
    btn_east.default_symbol = "O";
    btn_east.description = _( "Olhar / Cancelar" );
    btn_east.border_r = 255; btn_east.border_g = 70; btn_east.border_b = 70;
    fixed_buttons.push_back( btn_east );

    touch_button btn_west; // Inventory / Drop
    btn_west.id = button_id::FACE_WEST;
    btn_west.style = button_visual_style::CIRCLE;
    btn_west.game_action = ACTION_INVENTORY;
    btn_west.game_fallback_key = 'i';
    btn_west.menu_fallback_key = 'd';
    btn_west.default_symbol = "[]";
    btn_west.description = _( "Mochila / Largar" );
    btn_west.border_r = 240; btn_west.border_g = 90; btn_west.border_b = 240;
    fixed_buttons.push_back( btn_west );

    touch_button btn_north; // Crafting & Build / Details
    btn_north.id = button_id::FACE_NORTH;
    btn_north.style = button_visual_style::CIRCLE;
    btn_north.game_action = ACTION_CRAFT;
    btn_north.game_fallback_key = '&';
    btn_north.menu_fallback_key = '?';
    btn_north.default_symbol = "^";
    btn_north.description = _( "Criacao / Detalhes" );
    btn_north.border_r = 90; btn_north.border_g = 240; btn_north.border_b = 130;
    fixed_buttons.push_back( btn_north );

    // 2. Shoulder Triggers
    touch_button btn_l1; // L1: Quick Weapon Swap / Prev Tab
    btn_l1.id = button_id::SHOULDER_L1;
    btn_l1.style = button_visual_style::PILL;
    btn_l1.game_action = ACTION_WIELD;
    btn_l1.game_fallback_key = 'w';
    btn_l1.menu_fallback_key = KEY_BTAB;
    btn_l1.default_symbol = "L1";
    btn_l1.description = _( "Troca de Arma / Aba Ant." );
    fixed_buttons.push_back( btn_l1 );

    touch_button btn_l2; // L2: Run/Walk/Crouch Toggle / Search Filter
    btn_l2.id = button_id::SHOULDER_L2;
    btn_l2.style = button_visual_style::PILL;
    btn_l2.game_action = ACTION_CYCLE_MOVE;
    btn_l2.game_fallback_key = '"';
    btn_l2.menu_fallback_key = '/';
    btn_l2.default_symbol = "L2";
    btn_l2.description = _( "Marcha / Buscar" );
    fixed_buttons.push_back( btn_l2 );

    touch_button btn_r1; // R1: Aim & Fire / Next Tab
    btn_r1.id = button_id::SHOULDER_R1;
    btn_r1.style = button_visual_style::PILL;
    btn_r1.game_action = ACTION_FIRE;
    btn_r1.game_fallback_key = 'f';
    btn_r1.menu_fallback_key = '\t';
    btn_r1.default_symbol = "R1";
    btn_r1.description = _( "Mirar / Prox. Aba" );
    fixed_buttons.push_back( btn_r1 );

    touch_button btn_r2; // R2: Fire Mode / Throw / Select All
    btn_r2.id = button_id::SHOULDER_R2;
    btn_r2.style = button_visual_style::PILL;
    btn_r2.game_action = ACTION_SELECT_FIRE_MODE;
    btn_r2.game_fallback_key = 'F';
    btn_r2.menu_fallback_key = ',';
    btn_r2.default_symbol = "R2";
    btn_r2.description = _( "Modo Tiro / Selecionar Tudo" );
    fixed_buttons.push_back( btn_r2 );

    // 3. Center Bottom Buttons
    touch_button btn_c1; // C1: Wait 1 Turn / Decrement
    btn_c1.id = button_id::CENTER_WAIT_1T;
    btn_c1.style = button_visual_style::ROUNDED_RECT;
    btn_c1.game_action = ACTION_PAUSE;
    btn_c1.game_fallback_key = '.';
    btn_c1.menu_fallback_key = '-';
    btn_c1.default_symbol = "1T";
    btn_c1.description = _( "Passar 1 Turno" );
    fixed_buttons.push_back( btn_c1 );

    touch_button btn_c2; // C2: Advanced Inventory / Sort
    btn_c2.id = button_id::CENTER_ADV_INV;
    btn_c2.style = button_visual_style::ROUNDED_RECT;
    btn_c2.game_action = ACTION_ADVANCEDINV;
    btn_c2.game_fallback_key = '/';
    btn_c2.menu_fallback_key = 's';
    btn_c2.default_symbol = "ADV";
    btn_c2.description = _( "Inventario Avancado / Ordenar" );
    fixed_buttons.push_back( btn_c2 );

    // 4. Top HUD Buttons
    touch_button btn_char; // Red Square: Character Sheet
    btn_char.id = button_id::TOP_CHAR_SHEET;
    btn_char.style = button_visual_style::ROUNDED_RECT;
    btn_char.game_action = ACTION_PL_INFO;
    btn_char.game_fallback_key = '@';
    btn_char.menu_fallback_key = '@';
    btn_char.default_symbol = "BIO";
    btn_char.description = _( "Ficha do Personagem" );
    btn_char.border_r = 230; btn_char.border_g = 50; btn_char.border_b = 50;
    fixed_buttons.push_back( btn_char );

    touch_button btn_map; // Contextual Map
    btn_map.id = button_id::TOP_MAP_BUTTON;
    btn_map.style = button_visual_style::ROUNDED_RECT;
    btn_map.game_action = ACTION_MAP;
    btn_map.game_fallback_key = 'm';
    btn_map.menu_fallback_key = 'm';
    btn_map.default_symbol = "MAP";
    btn_map.description = _( "Mapa Mundi" );
    fixed_buttons.push_back( btn_map );

    touch_button btn_hub; // Brown Square: App Hub & Settings
    btn_hub.id = button_id::TOP_HUB_MENU;
    btn_hub.style = button_visual_style::ROUNDED_RECT;
    btn_hub.game_action = ACTION_NULL;
    btn_hub.game_fallback_key = 0;
    btn_hub.menu_fallback_key = 0;
    btn_hub.default_symbol = "HUB";
    btn_hub.description = _( "Hub & Configuracoes" );
    btn_hub.border_r = 180; btn_hub.border_g = 110; btn_hub.border_b = 60;
    fixed_buttons.push_back( btn_hub );

    // 5. Dynamic Context Slots (Right: 3 slots)
    for( int i = 0; i < 3; ++i ) {
        touch_button slot;
        slot.id = static_cast<button_id>( static_cast<int>( button_id::CONTEXT_RIGHT_1 ) + i );
        slot.style = button_visual_style::ROUNDED_RECT;
        slot.is_visible = false;
        slot.border_r = 255; slot.border_g = 215; slot.border_b = 0; // Gold accent
        right_context_slots.push_back( slot );
    }

    // 6. Dynamic Context Slots (Left: 3 slots)
    for( int i = 0; i < 3; ++i ) {
        touch_button slot;
        slot.id = static_cast<button_id>( static_cast<int>( button_id::CONTEXT_LEFT_1 ) + i );
        slot.style = button_visual_style::ROUNDED_RECT;
        slot.is_visible = false;
        slot.border_r = 135; slot.border_g = 206; slot.border_b = 250; // Cyan accent
        left_context_slots.push_back( slot );
    }
}

bool mobile_ui_system::is_gameplay_ready() const
{
    return g != nullptr && g->is_core_data_loaded() && bool( g->w_terrain ) && !get_player_character().is_dead_state();
}

bool mobile_ui_system::is_menu_active() const
{
    input_context *top_ctx = input_context::input_context_stack.back();
    if( !top_ctx ) {
        return false;
    }
    const std::string &cat = top_ctx->get_category();
    return cat != "DEFAULTMODE";
}

bool mobile_ui_system::should_show_context_slots() const
{
    if( !is_gameplay_ready() ) {
        return false;
    }

    // 1. Gaveta expandida manualmente pelo jogador pelo botao lateral [<] / [>]
    if( context_drawer_expanded ) {
        return true;
    }

    // 2. Quando o jogador entra em telas contextuais (inventario, inspecionar/olhar, exame, livros, etc.)
    if( is_menu_active() ) {
        return true;
    }

    // 3. Emergencias criticas de combate e sobrevivencia (HP < 50%, sem municao na arma de fogo)
    try {
        Character &u = get_player_character();
        if( u.get_hp() <= ( u.get_hp_max() / 2 ) ) {
            return true;
        }
        item_location wielded = u.get_wielded_item();
        if( wielded && wielded->is_gun() && wielded->ammo_remaining() == 0 ) {
            return true;
        }
    } catch( ... ) {
    }

    return false;
}

void mobile_ui_system::update_layout( int window_w, int window_h )
{
    if( window_w <= 0 || window_h <= 0 ) {
        return;
    }
    if( window_w != screen_width || window_h != screen_height ) {
        screen_width = window_w;
        screen_height = window_h;
        calculate_geometry( window_w, window_h );
    }
}

void mobile_ui_system::calculate_geometry( int w, int h )
{
    screen_width = w;
    screen_height = h;

    // Escala de densidade adaptada para telas modernas (Poco X7 Pro, FHD+, 2K, etc.)
    // Poco X7 Pro: 2712 x 1220 => min_dim = 1220 => density_scale = ~1.90f
    float min_dim = static_cast<float>( std::min( w, h ) );
    float opt_density = get_option<float>( "MOBILE_UI_DENSITY_SCALE" );
    if( opt_density <= 0.1f ) {
        opt_density = 1.0f;
    }
    density_scale = std::clamp( ( min_dim / 640.0f ) * opt_density, 1.25f, 3.5f );

    // Margens de seguranca para evitar bordas de tela, cortes arredondados e barra de gestos do HyperOS
    float safe_margin_x = 36.0f * density_scale;
    float safe_margin_bottom = 32.0f * density_scale;
    float safe_margin_top = 18.0f * density_scale;

    float dpad_scale = get_option<float>( "MOBILE_DPAD_SCALE" );
    if( dpad_scale <= 0.1f ) { dpad_scale = 1.0f; }

    float face_scale = get_option<float>( "MOBILE_FACE_SCALE" );
    if( face_scale <= 0.1f ) { face_scale = 1.0f; }

    float shoulder_scale = get_option<float>( "MOBILE_SHOULDER_SCALE" );
    if( shoulder_scale <= 0.1f ) { shoulder_scale = 1.0f; }

    float context_scale = get_option<float>( "MOBILE_CONTEXT_SCALE" );
    if( context_scale <= 0.1f ) { context_scale = 1.0f; }

    float center_scale = get_option<float>( "MOBILE_CENTER_SCALE" );
    if( center_scale <= 0.1f ) { center_scale = 1.0f; }

    int dpad_ox = get_option<int>( "MOBILE_DPAD_POS_X" );
    int dpad_oy = get_option<int>( "MOBILE_DPAD_POS_Y" );
    int face_ox = get_option<int>( "MOBILE_FACE_POS_X" );
    int face_oy = get_option<int>( "MOBILE_FACE_POS_Y" );

    bool swap_sides = get_option<bool>( "MOBILE_SWAP_SIDES" );

    // 1. Exiled Kingdoms Analog Pad (Inferior Esquerdo por padrao)
    analog_radius = 58.0f * density_scale * dpad_scale;
    float analog_base_x = safe_margin_x + analog_radius + static_cast<float>( dpad_ox );
    float analog_base_y = static_cast<float>( h ) - safe_margin_bottom - analog_radius + static_cast<float>( dpad_oy );
    if( swap_sides ) {
        analog_base_x = static_cast<float>( w ) - safe_margin_x - analog_radius - static_cast<float>( dpad_ox );
    }
    analog_center = point( static_cast<int>( analog_base_x ), static_cast<int>( analog_base_y ) );

    // 2. PPSSPP Diamond Face Buttons (Inferior Direito por padrao)
    int btn_size = static_cast<int>( 52.0f * density_scale * face_scale );
    int diamond_radius = static_cast<int>( 56.0f * density_scale * face_scale );

    float face_base_x = static_cast<float>( w ) - safe_margin_x - diamond_radius - ( btn_size / 2.0f ) + static_cast<float>( face_ox );
    float face_base_y = static_cast<float>( h ) - safe_margin_bottom - diamond_radius - ( btn_size / 2.0f ) + static_cast<float>( face_oy );
    if( swap_sides ) {
        face_base_x = safe_margin_x + diamond_radius + ( btn_size / 2.0f ) - static_cast<float>( face_ox );
    }

    // Top HUD Button sizes
    int hub_btn_size = static_cast<int>( 46.0f * density_scale );
    int top_y = static_cast<int>( safe_margin_top );

    // Shoulder Buttons
    int sw = static_cast<int>( 72.0f * density_scale * shoulder_scale );
    int sh = static_cast<int>( 36.0f * density_scale * shoulder_scale );
    int shoulder_y = top_y + hub_btn_size + static_cast<int>( 10.0f * density_scale );

    // Center Bottom Buttons
    int cw = static_cast<int>( 68.0f * density_scale * center_scale );
    int ch = static_cast<int>( 34.0f * density_scale * center_scale );
    int center_y = h - static_cast<int>( safe_margin_bottom ) - ch;

    for( auto &btn : fixed_buttons ) {
        switch( btn.id ) {
            case button_id::FACE_SOUTH:
                btn.bounds = { static_cast<int>( face_base_x - btn_size / 2 ),
                               static_cast<int>( face_base_y + diamond_radius - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_EAST:
                btn.bounds = { static_cast<int>( face_base_x + diamond_radius - btn_size / 2 ),
                               static_cast<int>( face_base_y - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_WEST:
                btn.bounds = { static_cast<int>( face_base_x - diamond_radius - btn_size / 2 ),
                               static_cast<int>( face_base_y - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_NORTH:
                btn.bounds = { static_cast<int>( face_base_x - btn_size / 2 ),
                               static_cast<int>( face_base_y - diamond_radius - btn_size / 2 ),
                               btn_size, btn_size };
                break;

            // Shoulder Triggers
            case button_id::SHOULDER_L1:
                btn.bounds = { static_cast<int>( safe_margin_x ), shoulder_y, sw, sh };
                break;
            case button_id::SHOULDER_L2:
                btn.bounds = { static_cast<int>( safe_margin_x + sw + 10 ), shoulder_y, sw, sh };
                break;
            case button_id::SHOULDER_R1:
                btn.bounds = { static_cast<int>( w - safe_margin_x - sw * 2 - 10 ), shoulder_y, sw, sh };
                break;
            case button_id::SHOULDER_R2:
                btn.bounds = { static_cast<int>( w - safe_margin_x - sw ), shoulder_y, sw, sh };
                break;

            // Bottom Center Buttons
            case button_id::CENTER_WAIT_1T:
                btn.bounds = { w / 2 - cw - 8, center_y, cw, ch };
                break;
            case button_id::CENTER_ADV_INV:
                btn.bounds = { w / 2 + 8, center_y, cw, ch };
                break;

            // Top HUD Buttons
            case button_id::TOP_CHAR_SHEET:
                btn.bounds = { static_cast<int>( safe_margin_x ), top_y, hub_btn_size, hub_btn_size };
                break;
            case button_id::TOP_MAP_BUTTON:
                btn.bounds = { static_cast<int>( w - safe_margin_x - hub_btn_size * 2 - 12 ), top_y, hub_btn_size, hub_btn_size };
                break;
            case button_id::TOP_HUB_MENU: // Hub Marrom
                btn.bounds = { static_cast<int>( w - safe_margin_x - hub_btn_size ), top_y, hub_btn_size, hub_btn_size };
                break;

            default:
                break;
        }
    }

    // Dynamic Context Slots (Escada ao redor dos polegares)
    int slot_w = static_cast<int>( 88.0f * density_scale * context_scale );
    int slot_h = static_cast<int>( 36.0f * density_scale * context_scale );
    int slot_gap = static_cast<int>( 8.0f * density_scale );

    if( !swap_sides ) {
        // Right Staircase (Thumb direito)
        right_context_slots[0].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w - slot_gap ),
                                          static_cast<int>( face_base_y + slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[1].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w ),
                                          static_cast<int>( face_base_y - slot_h - slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[2].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w + static_cast<int>( 20.0f * density_scale ) ),
                                          static_cast<int>( face_base_y - slot_h * 2 - slot_gap * 2 ),
                                          slot_w, slot_h };

        // Left Staircase (Thumb esquerdo)
        left_context_slots[0].bounds = { static_cast<int>( analog_base_x + analog_radius + slot_gap ),
                                         static_cast<int>( analog_base_y - slot_h / 2 ),
                                         slot_w, slot_h };
        left_context_slots[1].bounds = { static_cast<int>( analog_base_x + analog_radius ),
                                         static_cast<int>( analog_base_y - slot_h * 2 - slot_gap ),
                                         slot_w, slot_h };
        left_context_slots[2].bounds = { static_cast<int>( analog_base_x + analog_radius - static_cast<int>( 15.0f * density_scale ) ),
                                         static_cast<int>( analog_base_y - slot_h * 3 - slot_gap * 2 ),
                                         slot_w, slot_h };
    } else {
        // Canhoto (inverteu analogico e face)
        right_context_slots[0].bounds = { static_cast<int>( face_base_x + diamond_radius + slot_gap ),
                                          static_cast<int>( face_base_y + slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[1].bounds = { static_cast<int>( face_base_x + diamond_radius ),
                                          static_cast<int>( face_base_y - slot_h - slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[2].bounds = { static_cast<int>( face_base_x + diamond_radius - static_cast<int>( 20.0f * density_scale ) ),
                                          static_cast<int>( face_base_y - slot_h * 2 - slot_gap * 2 ),
                                          slot_w, slot_h };

        left_context_slots[0].bounds = { static_cast<int>( analog_base_x - analog_radius - slot_w - slot_gap ),
                                         static_cast<int>( analog_base_y - slot_h / 2 ),
                                         slot_w, slot_h };
        left_context_slots[1].bounds = { static_cast<int>( analog_base_x - analog_radius - slot_w ),
                                         static_cast<int>( analog_base_y - slot_h * 2 - slot_gap ),
                                         slot_w, slot_h };
        left_context_slots[2].bounds = { static_cast<int>( analog_base_x - analog_radius - slot_w + static_cast<int>( 15.0f * density_scale ) ),
                                         static_cast<int>( analog_base_y - slot_h * 3 - slot_gap * 2 ),
                                         slot_w, slot_h };
    }

    // Aba retrÃ¡til da gaveta de contexto na borda direita
    int toggle_w = static_cast<int>( 28.0f * density_scale );
    int toggle_h = static_cast<int>( 60.0f * density_scale );
    int toggle_x = screen_width - toggle_w;
    int toggle_y = ( screen_height - toggle_h ) / 2;
    context_toggle_btn = { toggle_x, toggle_y, toggle_w, toggle_h };
}

void mobile_ui_system::update_context_actions()
{
    if( !is_gameplay_ready() ) {
        active_right_actions.clear();
        active_left_actions.clear();
        for( auto &slot : right_context_slots ) {
            slot.is_visible = false;
        }
        for( auto &slot : left_context_slots ) {
            slot.is_visible = false;
        }
        return;
    }

    try {
        active_right_actions.clear();
        active_left_actions.clear();

        Character &u = get_player_character();

        // 1. Right Staircase - Combat & Survival Actions
        // R1: Reload current weapon
        item_location wielded = u.get_wielded_item();
        if( wielded && wielded->is_gun() ) {
            context_action act_reload;
            act_reload.act_id = ACTION_RELOAD_WEAPON;
            act_reload.fallback_key = 'r';
            act_reload.default_symbol = "REC";
            act_reload.label = _( "Recarregar" );
            active_right_actions.push_back( act_reload );
        }

        // R2: Bandage / Medical emergency if wounded
        if( u.get_hp() < u.get_hp_max() ) {
            context_action act_heal;
            act_heal.act_id = ACTION_USE;
            act_heal.fallback_key = 'a';
            act_heal.default_symbol = "CURA";
            act_heal.label = _( "Curativo" );
            active_right_actions.push_back( act_heal );
        }

        // R3: Open / Close Doors or Grab
        context_action act_door;
        act_door.act_id = ACTION_OPEN;
        act_door.fallback_key = 'o';
        act_door.default_symbol = "PORT";
        act_door.label = _( "Abrir/Fechar" );
        active_right_actions.push_back( act_door );

        // 2. Left Staircase - Exploration & Utilities
        // L1: Flashlight / Light toggle
        context_action act_light;
        act_light.act_id = ACTION_USE;
        act_light.fallback_key = 'a';
        act_light.default_symbol = "LUZ";
        act_light.label = _( "Lanterna" );
        active_left_actions.push_back( act_light );

        // L2: Sleep / Rest if tired
        if( u.get_sleepiness() >= 200 ) {
            context_action act_sleep;
            act_sleep.act_id = ACTION_SLEEP;
            act_sleep.fallback_key = '$';
            act_sleep.default_symbol = "SONO";
            act_sleep.label = _( "Dormir" );
            active_left_actions.push_back( act_sleep );
        }

        // L3: Vehicle controls or Read
        context_action act_read;
        act_read.act_id = ACTION_READ;
        act_read.fallback_key = 'R';
        act_read.default_symbol = "LER";
        act_read.label = _( "Ler Livro" );
        active_left_actions.push_back( act_read );

        // Bind right actions to slots
        for( size_t i = 0; i < right_context_slots.size(); ++i ) {
            if( i < active_right_actions.size() ) {
                right_context_slots[i].is_visible = true;
                right_context_slots[i].game_action = active_right_actions[i].act_id;
                right_context_slots[i].game_fallback_key = active_right_actions[i].fallback_key;
                right_context_slots[i].default_symbol = active_right_actions[i].default_symbol;
                right_context_slots[i].description = active_right_actions[i].label;
            } else {
                right_context_slots[i].is_visible = false;
            }
        }

        // Bind left actions to slots
        for( size_t i = 0; i < left_context_slots.size(); ++i ) {
            if( i < active_left_actions.size() ) {
                left_context_slots[i].is_visible = true;
                left_context_slots[i].game_action = active_left_actions[i].act_id;
                left_context_slots[i].game_fallback_key = active_left_actions[i].fallback_key;
                left_context_slots[i].default_symbol = active_left_actions[i].default_symbol;
                left_context_slots[i].description = active_left_actions[i].label;
            } else {
                left_context_slots[i].is_visible = false;
            }
        }
    } catch( ... ) {
        // Prevencao de qualquer excecao em contexto
    }
}

std::string mobile_ui_system::get_button_display_text( const touch_button &btn ) const
{
    if( !btn.custom_emoji.empty() ) {
        return btn.custom_emoji;
    }
    return btn.default_symbol;
}

void mobile_ui_system::draw_glass_button( const SDL_Renderer_Ptr &renderer, Font *font,
                                         const touch_button &btn )
{
    if( !btn.is_visible ) {
        return;
    }

    const int alpha_opt = get_option<int>( "MOBILE_UI_OPACITY" );
    const float alpha_norm = std::clamp( alpha_opt, 10, 100 ) / 100.0f;
    const bool high_contrast = get_option<bool>( "MOBILE_HIGH_CONTRAST" );

    // 1. Fundo de vidro fosco
    Uint8 bg_a = static_cast<Uint8>( 160 * alpha_norm );
    if( btn.is_pressed ) {
        bg_a = static_cast<Uint8>( 235 * alpha_norm );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, btn.bg_r, btn.bg_g, btn.bg_b, bg_a );
    RenderFillRect( renderer, &btn.bounds );

    // 2. Contorno
    Uint8 b_a = static_cast<Uint8>( 220 * alpha_norm );
    if( high_contrast ) {
        SetRenderDrawColor( renderer, 255, 255, 0, 255 );
        RenderDrawRect( renderer, &btn.bounds );
        SDL_Rect inner = { btn.bounds.x + 1, btn.bounds.y + 1, btn.bounds.w - 2, btn.bounds.h - 2 };
        RenderDrawRect( renderer, &inner );
    } else {
        SetRenderDrawColor( renderer, btn.border_r, btn.border_g, btn.border_b, b_a );
        RenderDrawRect( renderer, &btn.bounds );
    }

    // 3. Simbolo / Texto
    if( font ) {
        std::string text = get_button_display_text( btn );
        int text_len = utf8_width( text );
        int text_x = btn.bounds.x + ( btn.bounds.w - ( font->width * text_len ) ) / 2;
        int text_y = btn.bounds.y + ( btn.bounds.h - font->height ) / 2;

        font->OutputChar( renderer, text, point( text_x + 1, text_y + 1 ), 0, 0.6f );
        font->OutputChar( renderer, text, point( text_x, text_y ), 15, 1.0f );
    }
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw_exiled_analog( const SDL_Renderer_Ptr &renderer, Font *font )
{
    const int alpha_opt = get_option<int>( "MOBILE_UI_OPACITY" );
    const float alpha_norm = std::clamp( alpha_opt, 10, 100 ) / 100.0f;
    const bool high_contrast = get_option<bool>( "MOBILE_HIGH_CONTRAST" );

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    // Outer Circle Ring (Analog Base)
    SDL_Rect outer_box = {
        static_cast<int>( analog_center.x - analog_radius ),
        static_cast<int>( analog_center.y - analog_radius ),
        static_cast<int>( analog_radius * 2 ),
        static_cast<int>( analog_radius * 2 )
    };

    Uint8 base_a = static_cast<Uint8>( 75 * alpha_norm );
    SetRenderDrawColor( renderer, 25, 30, 40, base_a );
    RenderFillRect( renderer, &outer_box );

    if( high_contrast ) {
        SetRenderDrawColor( renderer, 255, 235, 59, 255 );
        RenderDrawRect( renderer, &outer_box );
    } else {
        SetRenderDrawColor( renderer, 255, 193, 7, static_cast<Uint8>( 160 * alpha_norm ) ); // Ambar fosco
        RenderDrawRect( renderer, &outer_box );
    }

    // Inner Floating Stick Knob (Puck)
    float knob_x = analog_center.x + analog_knob_x;
    float knob_y = analog_center.y + analog_knob_y;
    float knob_radius = analog_radius * 0.38f;

    SDL_Rect knob_box = {
        static_cast<int>( knob_x - knob_radius ),
        static_cast<int>( knob_y - knob_radius ),
        static_cast<int>( knob_radius * 2 ),
        static_cast<int>( knob_radius * 2 )
    };

    Uint8 knob_a = static_cast<Uint8>( ( analog_active ? 220 : 150 ) * alpha_norm );
    SetRenderDrawColor( renderer, 255, 193, 7, knob_a );
    RenderFillRect( renderer, &knob_box );
    SetRenderDrawColor( renderer, 255, 255, 255, 240 );
    RenderDrawRect( renderer, &knob_box );

    if( font && !analog_active ) {
        std::string center_hint = ".";
        int tx = analog_center.x - font->width / 2;
        int ty = analog_center.y - font->height / 2;
        font->OutputChar( renderer, center_hint, point( tx, ty ), 15 );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw_vital_stats_bar( const SDL_Renderer_Ptr &renderer, Font *font )
{
    if( !is_gameplay_ready() || !font ) {
        return;
    }

    float safe_margin_x = 36.0f * density_scale;
    float safe_margin_top = 18.0f * density_scale;
    int hub_btn_size = static_cast<int>( 46.0f * density_scale );

    // Posicionada logo ao lado do botao de BIO
    int bar_x = static_cast<int>( safe_margin_x ) + hub_btn_size + 10;
    int bar_y = static_cast<int>( safe_margin_top );
    int bar_w = static_cast<int>( 200.0f * density_scale );
    int bar_h = hub_btn_size;

    SDL_Rect bar_rect = { bar_x, bar_y, bar_w, bar_h };

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, 12, 22, 16, 170 );
    RenderFillRect( renderer, &bar_rect );

    SetRenderDrawColor( renderer, 46, 204, 113, 230 );
    RenderDrawRect( renderer, &bar_rect );

    try {
        Character &u = get_player_character();
        int hp_pct = 100;
        int max_hp = u.get_hp_max();
        if( max_hp > 0 ) {
            hp_pct = std::clamp( ( u.get_hp() * 100 ) / max_hp, 0, 100 );
        }
        int sta_pct = std::clamp( ( u.get_stamina() * 100 ) / std::max( 1, u.get_stamina_max() ), 0, 100 );

        int half_h = ( bar_rect.h - 8 ) / 2;

        // Mini HP Fill Bar
        SDL_Rect hp_fill = { bar_rect.x + 4, bar_rect.y + 3, ( bar_rect.w - 8 ) * hp_pct / 100, half_h };
        SetRenderDrawColor( renderer, 231, 76, 60, 200 );
        RenderFillRect( renderer, &hp_fill );

        // Mini Stamina Fill Bar
        SDL_Rect sta_fill = { bar_rect.x + 4, bar_rect.y + 4 + half_h, ( bar_rect.w - 8 ) * sta_pct / 100, half_h };
        SetRenderDrawColor( renderer, 52, 152, 219, 200 );
        RenderFillRect( renderer, &sta_fill );

        // Text Overlay
        std::string hp_str = "HP " + std::to_string( hp_pct ) + "%";
        std::string sta_str = "STA " + std::to_string( sta_pct ) + "%";
        font->OutputChar( renderer, hp_str, point( bar_rect.x + 8, bar_rect.y + 3 ), 15 );
        font->OutputChar( renderer, sta_str, point( bar_rect.x + 8, bar_rect.y + 4 + half_h ), 15 );
    } catch( ... ) {
        // Safe guard
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw( const SDL_Renderer_Ptr &renderer, Font *font )
{
    if( !initialized ) {
        init();
    }

    // Modo de posicionamento de gatilhos na tela (arrastar e soltar)
    if( is_positioning_triggers ) {
        draw_positioning_mode( renderer, font );
        return;
    }

    const bool gameplay_ready = is_gameplay_ready();

    bool cur_show_analog = true;
    bool cur_show_fixed = true;
    bool cur_show_context = true;

    // Se estiver com esquema customizado ativo, checa visibilidade e desenha os gatilhos
    if( active_applied_scheme_idx > 0 && active_applied_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        auto &scheme = loaded_schemes[active_applied_scheme_idx];
        cur_show_analog = scheme.show_analog;
        cur_show_fixed = scheme.show_fixed_buttons;
        cur_show_context = scheme.show_context_slots;

        for( auto &trig : scheme.triggers ) {
            trig.calculated_bounds.w = static_cast<int>( trig.size * density_scale );
            trig.calculated_bounds.h = static_cast<int>( trig.size * density_scale );
            trig.calculated_bounds.x = static_cast<int>( trig.x_pct * screen_width ) - trig.calculated_bounds.w / 2;
            trig.calculated_bounds.y = static_cast<int>( trig.y_pct * screen_height ) - trig.calculated_bounds.h / 2;
            draw_custom_trigger_button( renderer, font, trig, false );
        }
    }

    if( gameplay_ready && cur_show_context ) {
        update_context_actions();

        // 1. Barra de status do personagem: aparece apenas em sessao de jogo ativo
        draw_vital_stats_bar( renderer, font );
    }

    // 2. Botoes fixos (Face, Gatilhos, Centrais, Top HUD):
    // REGRA DE OURO: O botao TOP_HUB_MENU eh OBRIGATORIAMENTE mantido em tela 100% do tempo!
    for( const auto &btn : fixed_buttons ) {
        if( btn.id == button_id::TOP_HUB_MENU ) {
            draw_glass_button( renderer, font, btn );
            continue;
        }
        if( !cur_show_fixed ) {
            continue; // Deletado/limpo pelo esquema do usuario
        }
        if( btn.id == button_id::TOP_CHAR_SHEET && !gameplay_ready ) {
            continue;
        }
        draw_glass_button( renderer, font, btn );
    }

    // 3. Dynamic Context Slots & Aba Retratil (apenas se ativos no esquema)
    if( gameplay_ready && cur_show_context ) {
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
        SetRenderDrawColor( renderer, 15, 20, 28, 175 );
        RenderFillRect( renderer, &context_toggle_btn );

        if( should_show_context_slots() ) {
            SetRenderDrawColor( renderer, 255, 215, 0, 220 );
        } else {
            SetRenderDrawColor( renderer, 180, 195, 210, 150 );
        }
        RenderDrawRect( renderer, &context_toggle_btn );

        if( font ) {
            std::string arrow = should_show_context_slots() ? ">" : "<";
            int text_x = context_toggle_btn.x + ( context_toggle_btn.w - font->width ) / 2;
            int text_y = context_toggle_btn.y + ( context_toggle_btn.h - font->height ) / 2;
            font->OutputChar( renderer, arrow, point( text_x, text_y ), 15, 1.0f );
        }
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );

        if( should_show_context_slots() ) {
            for( const auto &slot : right_context_slots ) {
                if( slot.is_visible ) {
                    draw_glass_button( renderer, font, slot );
                }
            }
            for( const auto &slot : left_context_slots ) {
                if( slot.is_visible ) {
                    draw_glass_button( renderer, font, slot );
                }
            }
        }
    }

    // 4. Exiled Kingdoms Analog D-Pad: desenhado apenas se nao foi limpo/deletado
    if( cur_show_analog ) {
        draw_exiled_analog( renderer, font );
    }

    // 5. Draw Settings Drawer se estiver aberto (Hub Marrom)
    if( settings_open ) {
        draw_settings_drawer( renderer, font );
        if( trigger_config_modal_open ) {
            draw_trigger_config_modal( renderer, font );
        } else if( save_scheme_modal_open ) {
            draw_save_scheme_modal( renderer, font );
        }
    }
}

bool mobile_ui_system::handle_touch_down( float x, float y, int finger_id, uint32_t ticks )
{
    if( is_positioning_triggers ) {
        return handle_positioning_touch_down( x, y, finger_id );
    }

    if( settings_open ) {
        // Interceptado pelo Hub de configuracoes
        return true;
    }

    // Se estiver com esquema customizado ativo, checa toque nos gatilhos
    if( active_applied_scheme_idx > 0 && active_applied_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        auto &scheme = loaded_schemes[active_applied_scheme_idx];
        for( auto &trig : scheme.triggers ) {
            if( x >= trig.calculated_bounds.x && x <= trig.calculated_bounds.x + trig.calculated_bounds.w &&
                y >= trig.calculated_bounds.y && y <= trig.calculated_bounds.y + trig.calculated_bounds.h ) {
                trig.is_pressed = true;
                trig.active_finger_id = finger_id;
                return true;
            }
        }
    }

    const bool gameplay_ready = is_gameplay_ready();

    bool cur_show_analog = true;
    bool cur_show_fixed = true;
    bool cur_show_context = true;

    if( active_applied_scheme_idx > 0 && active_applied_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        const auto &scheme = loaded_schemes[active_applied_scheme_idx];
        cur_show_analog = scheme.show_analog;
        cur_show_fixed = scheme.show_fixed_buttons;
        cur_show_context = scheme.show_context_slots;
    }

    // 1. Check Analog D-Pad touch (apenas se nao foi limpo/deletado)
    if( cur_show_analog ) {
        float dx = x - static_cast<float>( analog_center.x );
        float dy = y - static_cast<float>( analog_center.y );
        float dist = std::sqrt( dx * dx + dy * dy );
        if( dist <= analog_radius * 1.35f && !analog_active ) {
            analog_active = true;
            analog_finger_id = finger_id;
            analog_last_repeat = ticks;
            if( dist > 0.0f ) {
                float norm = std::min( 1.0f, dist / analog_radius );
                analog_knob_x = ( dx / dist ) * ( analog_radius * norm );
                analog_knob_y = ( dy / dist ) * ( analog_radius * norm );
            }
            return true;
        }
    }

    // 2. Check Fixed Buttons (TOP_HUB_MENU sempre 100% ativo; os outros apenas se cur_show_fixed)
    for( auto &btn : fixed_buttons ) {
        if( !btn.is_visible ) continue;
        if( btn.id != button_id::TOP_HUB_MENU && !cur_show_fixed ) continue;
        if( btn.id == button_id::TOP_CHAR_SHEET && !gameplay_ready ) continue;

        if( x >= btn.bounds.x && x <= btn.bounds.x + btn.bounds.w &&
            y >= btn.bounds.y && y <= btn.bounds.y + btn.bounds.h ) {
            btn.is_pressed = true;
            btn.active_finger_id = finger_id;
            btn.press_start_time = ticks;
            return true;
        }
    }

    // 3. Dynamic Context Slots e Aba Retratil (apenas se cur_show_context e em jogo ativo)
    if( gameplay_ready && cur_show_context ) {
        // Checar toque na mini-aba retratil da lateral direita (com folga confortavel para o dedo)
        int margin = static_cast<int>( 15.0f * density_scale );
        if( x >= context_toggle_btn.x - margin && x <= screen_width &&
            y >= context_toggle_btn.y - margin && y <= context_toggle_btn.y + context_toggle_btn.h + margin ) {
            toggle_context_drawer();
            return true;
        }

        // Processar slots contextuais apenas se a lateral estiver aberta/visivel
        if( should_show_context_slots() ) {
            for( auto &slot : right_context_slots ) {
                if( !slot.is_visible ) continue;
                if( x >= slot.bounds.x && x <= slot.bounds.x + slot.bounds.w &&
                    y >= slot.bounds.y && y <= slot.bounds.y + slot.bounds.h ) {
                    slot.is_pressed = true;
                    slot.active_finger_id = finger_id;
                    slot.press_start_time = ticks;
                    return true;
                }
            }
            for( auto &slot : left_context_slots ) {
                if( !slot.is_visible ) continue;
                if( x >= slot.bounds.x && x <= slot.bounds.x + slot.bounds.w &&
                    y >= slot.bounds.y && y <= slot.bounds.y + slot.bounds.h ) {
                    slot.is_pressed = true;
                    slot.active_finger_id = finger_id;
                    slot.press_start_time = ticks;
                    return true;
                }
            }
        }
    }

    return false;
}

bool mobile_ui_system::handle_touch_motion( float x, float y, int finger_id, uint32_t /*ticks*/ )
{
    if( is_positioning_triggers ) {
        return handle_positioning_touch_motion( x, y, finger_id );
    }

    if( settings_open ) {
        return true;
    }

    if( analog_active && finger_id == analog_finger_id ) {
        float dx = x - static_cast<float>( analog_center.x );
        float dy = y - static_cast<float>( analog_center.y );
        float dist = std::sqrt( dx * dx + dy * dy );
        if( dist > 0.0f ) {
            float clamp_dist = std::min( analog_radius, dist );
            analog_knob_x = ( dx / dist ) * clamp_dist;
            analog_knob_y = ( dy / dist ) * clamp_dist;

            float angle = std::atan2( dy, dx ) * 180.0f / static_cast<float>( M_PI );
            if( angle < 0.0f ) angle += 360.0f;

            // 8-way directional mapping
            if( angle >= 337.5f || angle < 22.5f ) {
                analog_current_direction_key = '6'; // East (l)
            } else if( angle >= 22.5f && angle < 67.5f ) {
                analog_current_direction_key = '3'; // South-East (n)
            } else if( angle >= 67.5f && angle < 112.5f ) {
                analog_current_direction_key = '2'; // South (j)
            } else if( angle >= 112.5f && angle < 157.5f ) {
                analog_current_direction_key = '1'; // South-West (b)
            } else if( angle >= 157.5f && angle < 202.5f ) {
                analog_current_direction_key = '4'; // West (h)
            } else if( angle >= 202.5f && angle < 247.5f ) {
                analog_current_direction_key = '7'; // North-West (y)
            } else if( angle >= 247.5f && angle < 292.5f ) {
                analog_current_direction_key = '8'; // North (k)
            } else {
                analog_current_direction_key = '9'; // North-East (u)
            }
        }
        return true;
    }
    return false;
}

bool mobile_ui_system::handle_touch_up( float x, float y, int finger_id, uint32_t ticks,
                                        input_event &out_event )
{
    if( is_positioning_triggers ) {
        return handle_positioning_touch_up( x, y, finger_id );
    }

    if( settings_open ) {
        return handle_settings_touch( x, y, out_event );
    }

    // Se estiver com esquema customizado ativo, dispara o gatilho tocado
    if( active_applied_scheme_idx > 0 && active_applied_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        auto &scheme = loaded_schemes[active_applied_scheme_idx];
        for( auto &trig : scheme.triggers ) {
            if( trig.is_pressed && trig.active_finger_id == finger_id ) {
                trig.is_pressed = false;
                trig.active_finger_id = -1;
                if( !trig.keys.empty() ) {
                    out_event = input_event( trig.keys[0], input_event_t::keyboard_char );
                    pending_macro_events.clear();
                    for( size_t k = 1; k < trig.keys.size(); ++k ) {
                        pending_macro_events.push_back( input_event( trig.keys[k], input_event_t::keyboard_char ) );
                    }
                    return true;
                }
            }
        }
    }

    const bool gameplay_ready = is_gameplay_ready();

    // 1. Release Analog
    if( analog_active && finger_id == analog_finger_id ) {
        analog_active = false;
        analog_finger_id = -1;
        analog_knob_x = 0.0f;
        analog_knob_y = 0.0f;

        // Central quick tap passes 1 turn
        if( ticks - analog_last_repeat < 200 && std::abs( analog_knob_x ) < 15.0f && std::abs( analog_knob_y ) < 15.0f ) {
            out_event = input_event( '.', input_event_t::keyboard_char );
            return true;
        }
        analog_current_direction_key = 0;
        return true;
    }

    bool cur_show_fixed = true;
    if( active_applied_scheme_idx > 0 && active_applied_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        cur_show_fixed = loaded_schemes[active_applied_scheme_idx].show_fixed_buttons;
    }

    const bool in_menu = is_menu_active() || !gameplay_ready;

    // 2. Check Fixed Buttons
    for( auto &btn : fixed_buttons ) {
        if( btn.id != button_id::TOP_HUB_MENU && !cur_show_fixed ) {
            continue;
        }
        if( btn.is_pressed && btn.active_finger_id == finger_id ) {
            btn.is_pressed = false;
            btn.active_finger_id = -1;

            if( btn.id == button_id::TOP_HUB_MENU ) {
                toggle_settings_drawer();
                return true;
            }

            if( btn.id == button_id::TOP_CHAR_SHEET && !gameplay_ready ) {
                return false;
            }

            int key = in_menu ? btn.menu_fallback_key : btn.game_fallback_key;
            if( key != 0 ) {
                out_event = input_event( key, input_event_t::keyboard_char );
                return true;
            }
        }
    }

    // 3. Dynamic Context Slots (apenas se a lateral estiver aberta/visivel)
    if( gameplay_ready && should_show_context_slots() ) {
        for( auto &slot : right_context_slots ) {
            if( slot.is_pressed && slot.active_finger_id == finger_id ) {
                slot.is_pressed = false;
                slot.active_finger_id = -1;
                int key = slot.game_fallback_key;
                if( key != 0 ) {
                    out_event = input_event( key, input_event_t::keyboard_char );
                    return true;
                }
            }
        }
        for( auto &slot : left_context_slots ) {
            if( slot.is_pressed && slot.active_finger_id == finger_id ) {
                slot.is_pressed = false;
                slot.active_finger_id = -1;
                int key = slot.game_fallback_key;
                if( key != 0 ) {
                    out_event = input_event( key, input_event_t::keyboard_char );
                    return true;
                }
            }
        }
    }

    return false;
}

bool mobile_ui_system::handle_touch_repeat( uint32_t ticks, input_event &out_event )
{
    if( is_positioning_triggers || settings_open ) {
        return false;
    }

    // Despacha sequencialmente eventos pendentes de macro/combinacao de botoes
    if( !pending_macro_events.empty() ) {
        out_event = pending_macro_events.front();
        pending_macro_events.erase( pending_macro_events.begin() );
        return true;
    }

    if( analog_active && analog_current_direction_key != 0 ) {
        if( ticks - analog_last_repeat >= 180 ) {
            analog_last_repeat = ticks;
            out_event = input_event( analog_current_direction_key, input_event_t::keyboard_char );
            return true;
        }
    }
    return false;
}

void mobile_ui_system::toggle_settings_drawer()
{
    settings_open = !settings_open;
    if( settings_open ) {
        calculate_geometry( screen_width, screen_height );
    }
}

void mobile_ui_system::reset_to_defaults()
{
    get_options().get_option( "MOBILE_DPAD_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_FACE_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_SHOULDER_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_CONTEXT_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_CENTER_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_DPAD_POS_X" ).setValue( 0 );
    get_options().get_option( "MOBILE_DPAD_POS_Y" ).setValue( 0 );
    get_options().get_option( "MOBILE_FACE_POS_X" ).setValue( 0 );
    get_options().get_option( "MOBILE_FACE_POS_Y" ).setValue( 0 );
    get_options().get_option( "MOBILE_UI_OPACITY" ).setValue( 45 );
    get_options().get_option( "MOBILE_HIGH_CONTRAST" ).setValue( "false" );
    get_options().get_option( "MOBILE_SWAP_SIDES" ).setValue( "false" );
    get_options().get_option( "MOBILE_GLOBAL_FONT_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_GAMMA_BOOST" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_UI_DENSITY_SCALE" ).setValue( 1.0f );
    get_options().save();
    calculate_geometry( screen_width, screen_height );
}

void mobile_ui_system::draw_settings_drawer( const SDL_Renderer_Ptr &renderer, Font *font )
{
    // Hub ocupa 100% da tela disponivel (FULLSCREEN TOTAL)
    settings_box = { 0, 0, screen_width, screen_height };

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    // Fundo premium escuro estilo tatil/civilopedia
    SetRenderDrawColor( renderer, 12, 17, 26, 255 );
    RenderFillRect( renderer, &settings_box );

    // Borda ambar / marrom estilizada do Hub ao redor da tela
    SetRenderDrawColor( renderer, 195, 120, 65, 255 );
    RenderDrawRect( renderer, &settings_box );
    SDL_Rect inner_box = { 1, 1, screen_width - 2, screen_height - 2 };
    RenderDrawRect( renderer, &inner_box );

    if( !font ) {
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
        return;
    }

    // Cabecalho & Titulo
    std::string title = _( "HUB MOBILE & ACESSIBILIDADE - TOQUE NATIVO" );
    font->OutputChar( renderer, title, point( 28, 20 ), 11 );

    // Botao FECHAR no topo direito (Espacoso e confortavel para o polegar)
    settings_close_btn = { screen_width - 180, 12, 156, 44 };
    SetRenderDrawColor( renderer, 180, 45, 45, 240 );
    RenderFillRect( renderer, &settings_close_btn );
    SetRenderDrawColor( renderer, 255, 100, 100, 255 );
    RenderDrawRect( renderer, &settings_close_btn );

    std::string close_text = _( "[ X FECHAR ]" );
    int cl_tx = settings_close_btn.x + ( settings_close_btn.w - ( font->width * utf8_width( close_text ) ) ) / 2;
    int cl_ty = settings_close_btn.y + ( settings_close_btn.h - font->height ) / 2;
    font->OutputChar( renderer, close_text, point( cl_tx, cl_ty ), 15 );

    // 4 Abas Tatil Grandes com espacamento uniforme em 100% da largura
    int tab_gap = 14;
    int total_tabs_w = screen_width - 56;
    int tab_w = ( total_tabs_w - ( tab_gap * 3 ) ) / 4;
    int tab_h = 48;
    int tab_y = 66;

    std::array<std::string, 4> tab_labels = {
        _( "1. CONTROLES" ),
        _( "2. TELA/CORES" ),
        _( "3. ATALHOS & ESQUEMAS" ),
        _( "4. MODS" )
    };

    for( int i = 0; i < 4; ++i ) {
        settings_tabs[i] = { 28 + i * ( tab_w + tab_gap ), tab_y, tab_w, tab_h };
        bool is_active = ( settings_active_tab == i );

        if( is_active ) {
            SetRenderDrawColor( renderer, 41, 128, 185, 245 ); // Azul destaque
        } else {
            SetRenderDrawColor( renderer, 26, 36, 50, 220 ); // Cinza escuro inativo
        }
        RenderFillRect( renderer, &settings_tabs[i] );

        SetRenderDrawColor( renderer, is_active ? 255 : 90, is_active ? 215 : 120, is_active ? 0 : 150, 255 );
        RenderDrawRect( renderer, &settings_tabs[i] );

        int t_len = utf8_width( tab_labels[i] );
        int tx = settings_tabs[i].x + ( settings_tabs[i].w - ( font->width * t_len ) ) / 2;
        int ty = settings_tabs[i].y + ( settings_tabs[i].h - font->height ) / 2;
        font->OutputChar( renderer, tab_labels[i], point( tx, ty ), is_active ? 11 : 15 );
    }

    // Prepara a lista de opcoes da aba ativa
    current_tab_items.clear();

    if( settings_active_tab == 0 ) {
        // Layout e Tamanhos
        current_tab_items.push_back( { "MOBILE_DPAD_SCALE", _( "Tamanho do Analogico D-Pad" ), "float", 0.5f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_FACE_SCALE", _( "Tamanho dos Botoes de Acao (Losango)" ), "float", 0.5f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_SHOULDER_SCALE", _( "Tamanho dos Gatilhos Superiores L / R" ), "float", 0.5f, 2.0f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_CONTEXT_SCALE", _( "Tamanho dos Slots Contextuais" ), "float", 0.5f, 2.0f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_CENTER_SCALE", _( "Tamanho dos Botoes Centrais (1T / ADV)" ), "float", 0.5f, 2.0f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_SWAP_SIDES", _( "Modo Canhoto (Inverter Lados na Tela)" ), "bool", 0.0f, 1.0f, 1.0f } );
    } else if( settings_active_tab == 1 ) {
        // Acessibilidade e Visao
        current_tab_items.push_back( { "MOBILE_UI_OPACITY", _( "Opacidade Geral dos Botoes (Fundo)" ), "int", 10.0f, 100.0f, 5.0f } );
        current_tab_items.push_back( { "MOBILE_GLOBAL_FONT_SCALE", _( "Escala Global de Fontes da Interface" ), "float", 0.8f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_GAMMA_BOOST", _( "Brilho e Gama (Luz do Sol)" ), "float", 0.5f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_HIGH_CONTRAST", _( "Modo Alto Contraste (Contorno Neon Amarelo)" ), "bool", 0.0f, 1.0f, 1.0f } );
    }

    // Renderiza cada item da aba com botoes touch maiores e espacados (sem encavalar)
    int item_start_y = tab_y + tab_h + 20;
    int item_height = 54;
    int item_gap = 12;

    for( size_t i = 0; i < current_tab_items.size(); ++i ) {
        auto &item = current_tab_items[i];
        int iy = item_start_y + static_cast<int>( i ) * ( item_height + item_gap );

        // Fundo suave para a linha ocupando a tela inteira
        SDL_Rect row_rect = { 28, iy, screen_width - 56, item_height };
        SetRenderDrawColor( renderer, 20, 28, 42, 210 );
        RenderFillRect( renderer, &row_rect );
        SetRenderDrawColor( renderer, 45, 62, 86, 255 );
        RenderDrawRect( renderer, &row_rect );

        // Nome da opcao a esquerda
        font->OutputChar( renderer, item.title, point( row_rect.x + 18, iy + ( item_height - font->height ) / 2 ), 15 );

        if( item.type == "float" || item.type == "int" ) {
            // Botao [ - ]
            item.btn_minus = { row_rect.x + row_rect.w - 264, iy + 4, 64, item_height - 8 };
            SetRenderDrawColor( renderer, 45, 60, 85, 240 );
            RenderFillRect( renderer, &item.btn_minus );
            SetRenderDrawColor( renderer, 100, 140, 190, 255 );
            RenderDrawRect( renderer, &item.btn_minus );

            std::string minus_lbl = "-";
            int mx = item.btn_minus.x + ( item.btn_minus.w - font->width ) / 2;
            int my = item.btn_minus.y + ( item.btn_minus.h - font->height ) / 2;
            font->OutputChar( renderer, minus_lbl, point( mx, my ), 11 );

            // Valor formatado no centro
            std::string val_str;
            if( item.type == "float" ) {
                float val = get_option<float>( item.opt_name );
                std::ostringstream ss;
                ss << std::fixed << std::setprecision( 2 ) << val << "x";
                val_str = ss.str();
            } else {
                int val = get_option<int>( item.opt_name );
                val_str = std::to_string( val ) + "%";
            }

            int val_x = row_rect.x + row_rect.w - 192 + ( 110 - ( font->width * utf8_width( val_str ) ) ) / 2;
            int val_y = iy + ( item_height - font->height ) / 2;
            font->OutputChar( renderer, val_str, point( val_x, val_y ), 14 );

            // Botao [ + ]
            item.btn_plus = { row_rect.x + row_rect.w - 74, iy + 4, 64, item_height - 8 };
            SetRenderDrawColor( renderer, 45, 60, 85, 240 );
            RenderFillRect( renderer, &item.btn_plus );
            SetRenderDrawColor( renderer, 100, 140, 190, 255 );
            RenderDrawRect( renderer, &item.btn_plus );

            std::string plus_lbl = "+";
            int px = item.btn_plus.x + ( item.btn_plus.w - font->width ) / 2;
            int py = item.btn_plus.y + ( item.btn_plus.h - font->height ) / 2;
            font->OutputChar( renderer, plus_lbl, point( px, py ), 11 );

        } else if( item.type == "bool" ) {
            // Botao largo de Toggle Switch
            bool is_active = get_option<bool>( item.opt_name );
            item.btn_toggle = { row_rect.x + row_rect.w - 264, iy + 4, 254, item_height - 8 };

            if( is_active ) {
                SetRenderDrawColor( renderer, 39, 174, 96, 240 ); // Verde ativo
            } else {
                SetRenderDrawColor( renderer, 60, 70, 85, 240 ); // Cinza inativo
            }
            RenderFillRect( renderer, &item.btn_toggle );

            SetRenderDrawColor( renderer, is_active ? 46 : 90, is_active ? 204 : 100, is_active ? 113 : 110, 255 );
            RenderDrawRect( renderer, &item.btn_toggle );

            std::string toggle_text = is_active ? _( "[ ATIVADO ]" ) : _( "[ DESLIGADO ]" );
            int tg_x = item.btn_toggle.x + ( item.btn_toggle.w - ( font->width * utf8_width( toggle_text ) ) ) / 2;
            int tg_y = item.btn_toggle.y + ( item.btn_toggle.h - font->height ) / 2;
            font->OutputChar( renderer, toggle_text, point( tg_x, tg_y ), 15 );
        }
    }

    if( settings_active_tab == 3 ) {
        draw_mods_tab( renderer, font );
    } else if( settings_active_tab == 2 ) {
        draw_schemes_tab( renderer, font );
    } else if( false ) {
        // Aba de Instrucoes legada mantida como fallback
        int info_y = tab_y + tab_h + 20;
        font->OutputChar( renderer, _( "Instrucoes de Toque Puro (Touch-Native Android):" ), point( settings_box.x + 30, info_y ), 14 );
        font->OutputChar( renderer, _( "- Nao precisa digitar nada no teclado virtual (Gboard)!" ), point( settings_box.x + 30, info_y + 35 ), 15 );
        font->OutputChar( renderer, _( "- Toque direto nos botoes [+] e [-] para alterar o tamanho dos controles." ), point( settings_box.x + 30, info_y + 65 ), 15 );
        font->OutputChar( renderer, _( "- Todas as alteracoes sao salvas automaticamente no mesmo instante." ), point( settings_box.x + 30, info_y + 95 ), 15 );
        font->OutputChar( renderer, _( "- Em jogo, use o D-Pad para andar e os botoes direitos para combate/inventario." ), point( settings_box.x + 30, info_y + 125 ), 15 );

        // Botao de Reset central
        settings_reset_btn = { settings_box.x + ( settings_box.w - 420 ) / 2, info_y + 190, 420, 52 };
        SetRenderDrawColor( renderer, 192, 57, 43, 230 ); // Vermelho
        RenderFillRect( renderer, &settings_reset_btn );
        SetRenderDrawColor( renderer, 231, 76, 60, 255 );
        RenderDrawRect( renderer, &settings_reset_btn );

        std::string reset_lbl = _( "[ RESTAURAR CONFIGURACOES PADRAO ]" );
        int rx = settings_reset_btn.x + ( settings_reset_btn.w - ( font->width * utf8_width( reset_lbl ) ) ) / 2;
        int ry = settings_reset_btn.y + ( settings_reset_btn.h - font->height ) / 2;
        font->OutputChar( renderer, reset_lbl, point( rx, ry ), 15 );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

bool mobile_ui_system::handle_settings_touch( float x, float y, input_event &/*out_event*/ )
{
    // 1. Fechar botao
    if( x >= settings_close_btn.x && x <= settings_close_btn.x + settings_close_btn.w &&
        y >= settings_close_btn.y && y <= settings_close_btn.y + settings_close_btn.h ) {
        settings_open = false;
        return true;
    }

    // 2. Abas
    for( int i = 0; i < 4; ++i ) {
        if( x >= settings_tabs[i].x && x <= settings_tabs[i].x + settings_tabs[i].w &&
            y >= settings_tabs[i].y && y <= settings_tabs[i].y + settings_tabs[i].h ) {
            settings_active_tab = i;
            if( settings_active_tab == 3 ) {
                refresh_mods_list();
            }
            return true;
        }
    }

    // Se os sub-modais de gatilho ou salvar esquema estiverem abertos
    if( trigger_config_modal_open ) {
        return handle_trigger_config_touch( x, y );
    }
    if( save_scheme_modal_open ) {
        return handle_save_scheme_touch( x, y );
    }

    // Se estiver na Aba 3 (Atalhos & Esquemas), processa os toques de esquemas
    if( settings_active_tab == 2 ) {
        return handle_schemes_tab_touch( x, y );
    }

    // Se estiver na Aba 4 (Mods), processa os toques de mods
    if( settings_active_tab == 3 ) {
        return handle_mods_tab_touch( x, y );
    }

    // 3. Reset Button na Aba 2
    if( settings_active_tab == 2 ) {
        if( x >= settings_reset_btn.x && x <= settings_reset_btn.x + settings_reset_btn.w &&
            y >= settings_reset_btn.y && y <= settings_reset_btn.y + settings_reset_btn.h ) {
            reset_to_defaults();
            return true;
        }
    }

    // 4. Botoes tatil das opcoes [-], [+] e [TOGGLE]
    for( const auto &item : current_tab_items ) {
        if( item.type == "float" || item.type == "int" ) {
            // Toque no botao [ - ]
            if( x >= item.btn_minus.x && x <= item.btn_minus.x + item.btn_minus.w &&
                y >= item.btn_minus.y && y <= item.btn_minus.y + item.btn_minus.h ) {
                if( item.type == "float" ) {
                    float val = get_option<float>( item.opt_name );
                    val = std::clamp( val - item.step, item.min_val, item.max_val );
                    get_options().get_option( item.opt_name ).setValue( val );
                } else {
                    int val = get_option<int>( item.opt_name );
                    val = std::clamp( val - static_cast<int>( item.step ), static_cast<int>( item.min_val ), static_cast<int>( item.max_val ) );
                    get_options().get_option( item.opt_name ).setValue( val );
                }
                get_options().save();
                calculate_geometry( screen_width, screen_height );
                return true;
            }

            // Toque no botao [ + ]
            if( x >= item.btn_plus.x && x <= item.btn_plus.x + item.btn_plus.w &&
                y >= item.btn_plus.y && y <= item.btn_plus.y + item.btn_plus.h ) {
                if( item.type == "float" ) {
                    float val = get_option<float>( item.opt_name );
                    val = std::clamp( val + item.step, item.min_val, item.max_val );
                    get_options().get_option( item.opt_name ).setValue( val );
                } else {
                    int val = get_option<int>( item.opt_name );
                    val = std::clamp( val + static_cast<int>( item.step ), static_cast<int>( item.min_val ), static_cast<int>( item.max_val ) );
                    get_options().get_option( item.opt_name ).setValue( val );
                }
                get_options().save();
                calculate_geometry( screen_width, screen_height );
                return true;
            }
        } else if( item.type == "bool" ) {
            // Toque no Toggle
            if( x >= item.btn_toggle.x && x <= item.btn_toggle.x + item.btn_toggle.w &&
                y >= item.btn_toggle.y && y <= item.btn_toggle.y + item.btn_toggle.h ) {
                get_options().get_option( item.opt_name ).setNext();
                get_options().save();
                calculate_geometry( screen_width, screen_height );
                return true;
            }
        }
    }

    // 5. Toque fora do painel fecha o Hub
    if( x < settings_box.x || x > settings_box.x + settings_box.w ||
        y < settings_box.y || y > settings_box.y + settings_box.h ) {
        settings_open = false;
        return true;
    }

    return true;
}

void mobile_ui_system::set_button_emoji( button_id id, const std::string &emoji )
{
    for( auto &btn : fixed_buttons ) {
        if( btn.id == id ) {
            btn.custom_emoji = emoji;
            return;
        }
    }
}

void mobile_ui_system::refresh_mods_list()
{
    installed_mods.clear();
    community_mods.clear();

    std::set<std::string> active_ids;
    cata_path default_mods_file = PATH_INFO::mods_user_default();
    if( file_exist( default_mods_file ) ) {
        read_from_file_json( default_mods_file, [&]( const JsonValue &jv ) {
            JsonObject jo = jv.get_object();
            jo.allow_omitted_members();
            if( jo.has_array( "dependencies" ) ) {
                for( const std::string id : jo.get_array( "dependencies" ) ) {
                    active_ids.insert( id );
                }
            }
        } );
    }

    std::vector<std::string> search_dirs;
    search_dirs.push_back( PATH_INFO::user_moddir() );
    search_dirs.push_back( PATH_INFO::datadir() + "mods/" );

    std::set<std::string> seen_mod_ids;

    for( const auto &base_dir : search_dirs ) {
        if( !dir_exist( base_dir ) ) {
            continue;
        }

        try {
            for( const auto &entry : std::filesystem::directory_iterator( std::filesystem::u8path( base_dir ) ) ) {
                if( !entry.is_directory() ) {
                    continue;
                }
                std::string mod_dir = entry.path().u8string();
                std::string modinfo_path = mod_dir + "/modinfo.json";
                if( !file_exist( modinfo_path ) ) {
                    continue;
                }

                mobile_mod_item item;
                item.ident = entry.path().filename().u8string();
                item.name = item.ident;
                item.description = _( "Mod para Cataclysm: Dark Days Ahead." );
                item.version = "1.0";
                item.authors = "Comunidade";
                item.languages = "PT-BR, EN";
                item.compatibility = _( "Compativel (100%)" );
                item.tags = _( "[Conteudo]" );

                uintmax_t total_size = 0;
                try {
                    for( const auto &f : std::filesystem::recursive_directory_iterator( entry.path() ) ) {
                        if( f.is_regular_file() ) {
                            total_size += f.file_size();
                        }
                    }
                } catch( ... ) {}

                if( total_size > 1024 * 1024 ) {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision( 1 ) << ( total_size / ( 1024.0 * 1024.0 ) ) << " MB";
                    item.disk_size = ss.str();
                } else {
                    item.disk_size = std::to_string( total_size / 1024 ) + " KB";
                }

                try {
                    read_from_file_json( cata_path( cata_path::root_path::unknown, modinfo_path ), [&]( const JsonValue &jv ) {
                        JsonArray ja = jv.get_array();
                        for( const JsonValue entry_val : ja ) {
                            JsonObject mo = entry_val.get_object();
                            mo.allow_omitted_members();
                            if( mo.get_string( "type", "" ) == "MOD_INFO" ) {
                                if( mo.has_string( "id" ) ) {
                                    item.ident = mo.get_string( "id" );
                                }
                                if( mo.has_string( "name" ) ) {
                                    item.name = mo.get_string( "name" );
                                }
                                if( mo.has_string( "description" ) ) {
                                    item.description = mo.get_string( "description" );
                                }
                                if( mo.has_string( "version" ) ) {
                                    item.version = mo.get_string( "version" );
                                }
                                if( mo.has_string( "category" ) ) {
                                    item.tags = "[" + mo.get_string( "category" ) + "]";
                                }
                                if( mo.has_array( "authors" ) ) {
                                    std::string authors_str;
                                    for( const std::string auth : mo.get_array( "authors" ) ) {
                                        if( !authors_str.empty() ) authors_str += ", ";
                                        authors_str += auth;
                                    }
                                    item.authors = authors_str;
                                }
                                if( mo.has_array( "dependencies" ) ) {
                                    std::string deps_str;
                                    for( const std::string dep : mo.get_array( "dependencies" ) ) {
                                        if( !deps_str.empty() ) deps_str += ", ";
                                        deps_str += dep;
                                    }
                                    if( !deps_str.empty() ) {
                                        item.compatibility = _( "Requer: " ) + deps_str;
                                    }
                                }
                                break;
                            }
                        }
                    } );
                } catch( ... ) {}

                if( seen_mod_ids.count( item.ident ) == 0 ) {
                    seen_mod_ids.insert( item.ident );
                    item.is_active = ( active_ids.count( item.ident ) > 0 );
                    installed_mods.push_back( item );
                }
            }
        } catch( ... ) {}
    }

    std::vector<community_mod_item> catalog = {
        {
            "cdda-arcana-mod",
            "Arcana and Magic Items",
            "Chaosvolt",
            "Latest",
            _( "[Magia] [Itens] [Profissoes]" ),
            _( "Artes arcanas, cultos antigos, essencia magica e itens sobrenaturais balanceados para o apocalipse." ),
            "3.8 MB",
            "https://github.com/Chaosvolt/cdda-arcana-mod.git",
            false,
            { 0, 0, 0, 0 }
        },
        {
            "secronom",
            "Secronom - Debris of the Aftermath",
            "Axema",
            "v1.4",
            _( "[Inimigos] [Biomas] [Bosses]" ),
            _( "Nova ameaca biologica: criaturas carnosas mutantes, colossos mutantes e novos perigos organicos." ),
            "5.2 MB",
            "https://github.com/Axema/Secronom-Debris-of-the-Aftermath.git",
            false,
            { 0, 0, 0, 0 }
        },
        {
            "bombastic_perks",
            "Bombastic Perks",
            "BombasticSloth",
            "v2.1",
            _( "[RPG] [Talentos] [Progressao]" ),
            _( "Sistema completo de perks e talentos no estilo Fallout, ganhando vantagens exclusivas com a experiencia." ),
            "1.2 MB",
            "https://github.com/BombasticSloth/BombasticPerks.git",
            false,
            { 0, 0, 0, 0 }
        },
        {
            "more_survival_tools",
            "More Survival Tools",
            "Sir_Lagsalot",
            "v1.0",
            _( "[Sobrevivencia] [Crafting] [Itens]" ),
            _( "Ferramentas rusticas, purificadores de agua caseiros e armadilhas primitivas de sobrevivencia." ),
            "850 KB",
            "https://github.com/SirLagsalot/More-Survival-Tools.git",
            false,
            { 0, 0, 0, 0 }
        },
        {
            "tankmod",
            "Tankmod Revived",
            "CDDA Team",
            "0.H",
            _( "[Veiculos] [Militares] [Blindagem]" ),
            _( "Veiculos blindados pesados do exercito, canhoes funcionais e motores militares reforcados." ),
            "1.9 MB",
            "https://github.com/CleverRaven/Cataclysm-DDA.git",
            false,
            { 0, 0, 0, 0 }
        },
        {
            "DinoMod",
            "DinoMod",
            "Balthazar",
            "v1.8",
            _( "[Criaturas] [Jurassico] [Dinossauros]" ),
            _( "Animais pre-historicos e dinossauros clonados soltos nas matas e cidades apos o cataclismo." ),
            "2.1 MB",
            "https://github.com/CleverRaven/Cataclysm-DDA.git",
            false,
            { 0, 0, 0, 0 }
        }
    };

    for( auto &c_mod : catalog ) {
        c_mod.is_installed = ( seen_mod_ids.count( c_mod.ident ) > 0 );
        community_mods.push_back( c_mod );
    }
}

void mobile_ui_system::import_mods_from_disk()
{
    std::string dest_dir = PATH_INFO::user_moddir();
    assure_dir_exist( dest_dir );

    // Pastas de busca para importacao (tanto a interna quanto a publica em Documentos)
    std::vector<std::string> import_dirs = {
        "/storage/emulated/0/Documents/CataclysmDDA/import_mods/",
        "/storage/emulated/0/Documents/CataclysmDDA/mods/",
        PATH_INFO::user_dir() + "import_mods/"
    };

    // Tenta pre-criar as pastas publicas em Documentos para facilitar o acesso do jogador
    try {
        std::error_code ec;
        std::filesystem::create_directories( std::filesystem::u8path( "/storage/emulated/0/Documents/CataclysmDDA/import_mods" ), ec );
        std::filesystem::create_directories( std::filesystem::u8path( "/storage/emulated/0/Documents/CataclysmDDA/mods" ), ec );
        std::filesystem::create_directories( std::filesystem::u8path( "/storage/emulated/0/Documents/CataclysmDDA/save" ), ec );
    } catch( ... ) {}

    int imported_count = 0;

    for( const auto &import_dir : import_dirs ) {
        if( !dir_exist( import_dir ) ) {
            continue;
        }

        try {
            for( const auto &entry : std::filesystem::directory_iterator( std::filesystem::u8path( import_dir ) ) ) {
            if( entry.is_regular_file() ) {
                std::string ext = entry.path().extension().u8string();
                std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );

                if( ext == ".zip" ) {
                    std::string zip_file = entry.path().u8string();
                    std::string stem = entry.path().stem().u8string();
                    std::string target_folder = dest_dir + stem;
                    assure_dir_exist( target_folder );

#if defined(__ANDROID__) || defined(__linux__)
                    std::string cmd = "unzip -q -o \"" + zip_file + "\" -d \"" + target_folder + "\"";
                    int ret = system( cmd.c_str() );
                    if( ret == 0 ) {
                        imported_count++;
                    }
#endif
                } else if( ext == ".json" ) {
                    std::string stem = entry.path().stem().u8string();
                    std::string target_folder = dest_dir + stem;
                    assure_dir_exist( target_folder );
                    std::string target_file = target_folder + "/modinfo.json";
                    try {
                        std::filesystem::copy_file( entry.path(), std::filesystem::u8path( target_file ),
                                                    std::filesystem::copy_options::overwrite_existing );
                        imported_count++;
                    } catch( ... ) {}
                }
            } else if( entry.is_directory() ) {
                try {
                    std::string target_dir = dest_dir + entry.path().filename().u8string();
                    std::filesystem::copy( entry.path(), std::filesystem::u8path( target_dir ),
                                           std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing );
                    imported_count++;
                } catch( ... ) {}
            }
        }
        } catch( ... ) {}
    }

    refresh_mods_list();

    if( imported_count > 0 ) {
        mods_feedback_msg = string_format( _( "%d mod(s) importados para Documentos/CataclysmDDA/mods!" ), imported_count );
    } else {
        mods_feedback_msg = _( "Nenhum arquivo .zip ou .json encontrado na pasta Documentos/CataclysmDDA/import_mods" );
    }
}

void mobile_ui_system::toggle_mod_active_state( size_t index )
{
    if( index >= installed_mods.size() ) {
        return;
    }

    installed_mods[index].is_active = !installed_mods[index].is_active;

    cata_path default_mods_file = PATH_INFO::mods_user_default();
    write_to_file( default_mods_file, [&]( std::ostream &fout ) {
        JsonOut json( fout, true );
        json.start_object();
        json.member( "type", "MOD_INFO" );
        json.member( "id", "user:default" );
        json.member( "name", "User Default Mods" );
        json.member( "description", "Default mods enabled for newly created worlds" );
        json.member( "dependencies" );
        json.start_array();
        for( const auto &mod : installed_mods ) {
            if( mod.is_active ) {
                json.write( mod.ident );
            }
        }
        json.end_array();
        json.end_object();
    } );

    mods_feedback_msg = installed_mods[index].is_active ?
        string_format( _( "Mod '%s' [ATIVADO] para novos mundos!" ), installed_mods[index].name ) :
        string_format( _( "Mod '%s' [DESATIVADO]." ), installed_mods[index].name );
}

void mobile_ui_system::install_community_mod( size_t index )
{
    if( index >= community_mods.size() ) {
        return;
    }

    auto &c_mod = community_mods[index];
    std::string dest_folder = PATH_INFO::user_moddir() + c_mod.ident;
    assure_dir_exist( dest_folder );

    std::string data_mod_path = PATH_INFO::datadir() + "mods/" + c_mod.ident;
    if( dir_exist( data_mod_path ) ) {
        try {
            std::filesystem::copy( std::filesystem::u8path( data_mod_path ), std::filesystem::u8path( dest_folder ),
                                   std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing );
            c_mod.is_installed = true;
            mods_feedback_msg = string_format( _( "Mod '%s' instalado na pasta Documentos!" ), c_mod.name );
            refresh_mods_list();
            return;
        } catch( ... ) {}
    }

#if defined(__ANDROID__) || defined(__linux__)
    std::string cmd = "git clone --depth 1 " + c_mod.repo_url + " \"" + dest_folder + "\"";
    int ret = system( cmd.c_str() );
    if( ret == 0 ) {
        c_mod.is_installed = true;
        mods_feedback_msg = string_format( _( "Mod '%s' baixado e instalado com sucesso!" ), c_mod.name );
    } else {
        mods_feedback_msg = string_format( _( "Falha ao baixar mod '%s' do repositorio." ), c_mod.name );
    }
#endif

    refresh_mods_list();
}

void mobile_ui_system::draw_mods_tab( const SDL_Renderer_Ptr &renderer, Font *font )
{
    int top_y = settings_box.y + 115;

    // Sub-abas: [ Meus Mods ] e [ Repositorio Comunidade ]
    mods_btn_my_mods = { settings_box.x + 22, top_y, 230, 38 };
    mods_btn_community = { settings_box.x + 262, top_y, 280, 38 };
    mods_btn_import = { settings_box.x + 552, top_y, settings_box.w - 574, 38 };

    // Desenha botao [ Meus Mods ]
    bool sub_0_active = ( mods_subtab == 0 );
    SetRenderDrawColor( renderer, sub_0_active ? 41 : 28, sub_0_active ? 128 : 36, sub_0_active ? 185 : 48, 230 );
    RenderFillRect( renderer, &mods_btn_my_mods );
    SetRenderDrawColor( renderer, sub_0_active ? 100 : 70, sub_0_active ? 200 : 90, sub_0_active ? 255 : 120, 255 );
    RenderDrawRect( renderer, &mods_btn_my_mods );
    std::string my_mods_lbl = string_format( _( "MEUS MODS (%d)" ), static_cast<int>( installed_mods.size() ) );
    int mx = mods_btn_my_mods.x + ( mods_btn_my_mods.w - ( font->width * utf8_width( my_mods_lbl ) ) ) / 2;
    int my = mods_btn_my_mods.y + ( mods_btn_my_mods.h - font->height ) / 2;
    font->OutputChar( renderer, my_mods_lbl, point( mx, my ), sub_0_active ? 11 : 15 );

    // Desenha botao [ Repositorio Comunidade ]
    bool sub_1_active = ( mods_subtab == 1 );
    SetRenderDrawColor( renderer, sub_1_active ? 41 : 28, sub_1_active ? 128 : 36, sub_1_active ? 185 : 48, 230 );
    RenderFillRect( renderer, &mods_btn_community );
    SetRenderDrawColor( renderer, sub_1_active ? 100 : 70, sub_1_active ? 200 : 90, sub_1_active ? 255 : 120, 255 );
    RenderDrawRect( renderer, &mods_btn_community );
    std::string comm_lbl = string_format( _( "REPOSITORIO ONLINE (%d)" ), static_cast<int>( community_mods.size() ) );
    int cx = mods_btn_community.x + ( mods_btn_community.w - ( font->width * utf8_width( comm_lbl ) ) ) / 2;
    int cy = mods_btn_community.y + ( mods_btn_community.h - font->height ) / 2;
    font->OutputChar( renderer, comm_lbl, point( cx, cy ), sub_1_active ? 11 : 15 );

    // Desenha botao [ Importar .ZIP / .JSON ]
    SetRenderDrawColor( renderer, 39, 174, 96, 230 ); // Verde esmeralda
    RenderFillRect( renderer, &mods_btn_import );
    SetRenderDrawColor( renderer, 46, 204, 113, 255 );
    RenderDrawRect( renderer, &mods_btn_import );
    std::string imp_lbl = _( "[ IMPORTAR .ZIP / .JSON (import_mods) ]" );
    int ix = mods_btn_import.x + ( mods_btn_import.w - ( font->width * utf8_width( imp_lbl ) ) ) / 2;
    int iy = mods_btn_import.y + ( mods_btn_import.h - font->height ) / 2;
    font->OutputChar( renderer, imp_lbl, point( ix, iy ), 15 );

    // Linha de mensagem de feedback
    int msg_y = top_y + 44;
    std::string display_msg = mods_feedback_msg.empty() ?
        _( "Pasta de Mods: /storage/emulated/0/Documents/CataclysmDDA/mods" ) : mods_feedback_msg;
    font->OutputChar( renderer, display_msg, point( settings_box.x + 24, msg_y ), 14 );

    // Area dos cards da lista
    int list_y = msg_y + 26;
    int card_h = 72;
    int card_gap = 8;
    int visible_count = 3;

    if( mods_subtab == 0 ) {
        // Meus Mods Instalados
        if( installed_mods.empty() ) {
            std::string empty_1 = _( "Nenhum mod adicional encontrado na pasta Documentos/CataclysmDDA/mods." );
            std::string empty_2 = _( "Coloque arquivos .zip ou .json na pasta import_mods e toque em [IMPORTAR] acima!" );
            font->OutputChar( renderer, empty_1, point( settings_box.x + 30, list_y + 30 ), 15 );
            font->OutputChar( renderer, empty_2, point( settings_box.x + 30, list_y + 60 ), 11 );
        } else {
            int start_idx = std::clamp( mods_scroll_offset, 0, std::max( 0, static_cast<int>( installed_mods.size() ) - 1 ) );
            int end_idx = std::min( start_idx + visible_count, static_cast<int>( installed_mods.size() ) );

            for( int i = start_idx; i < end_idx; ++i ) {
                auto &mod = installed_mods[i];
                int cy_pos = list_y + ( i - start_idx ) * ( card_h + card_gap );

                SDL_Rect card_rect = { settings_box.x + 22, cy_pos, settings_box.w - 44, card_h };
                SetRenderDrawColor( renderer, 22, 28, 38, 220 );
                RenderFillRect( renderer, &card_rect );
                SetRenderDrawColor( renderer, 45, 60, 80, 255 );
                RenderDrawRect( renderer, &card_rect );

                // Linha 1: Nome, Versao, Tamanho, Idiomas
                std::string header_line = mod.name + " (" + mod.version + ")  |  " + mod.disk_size + "  |  " + mod.languages;
                font->OutputChar( renderer, header_line, point( card_rect.x + 12, card_rect.y + 6 ), 11 );

                // Linha 2: Tags e Compatibilidade
                std::string meta_line = mod.tags + "  " + mod.compatibility;
                font->OutputChar( renderer, meta_line, point( card_rect.x + 12, card_rect.y + 26 ), 10 );

                // Linha 3: Descricao concisa
                std::string desc_line = mod.description;
                if( utf8_width( desc_line ) > 60 ) {
                    desc_line = desc_line.substr( 0, 57 ) + "...";
                }
                font->OutputChar( renderer, desc_line, point( card_rect.x + 12, card_rect.y + 46 ), 15 );

                // Botao Toggle Ativo / Desativado
                mod.toggle_rect = { card_rect.x + card_rect.w - 190, card_rect.y + 12, 175, card_h - 24 };
                if( mod.is_active ) {
                    SetRenderDrawColor( renderer, 39, 174, 96, 240 ); // Verde Ativo
                } else {
                    SetRenderDrawColor( renderer, 55, 65, 80, 240 ); // Cinza Inativo
                }
                RenderFillRect( renderer, &mod.toggle_rect );
                SetRenderDrawColor( renderer, mod.is_active ? 46 : 90, mod.is_active ? 204 : 100, mod.is_active ? 113 : 110, 255 );
                RenderDrawRect( renderer, &mod.toggle_rect );

                std::string tog_lbl = mod.is_active ? _( "[ MOD ATIVO ]" ) : _( "[ DESATIVADO ]" );
                int tx = mod.toggle_rect.x + ( mod.toggle_rect.w - ( font->width * utf8_width( tog_lbl ) ) ) / 2;
                int ty = mod.toggle_rect.y + ( mod.toggle_rect.h - font->height ) / 2;
                font->OutputChar( renderer, tog_lbl, point( tx, ty ), 15 );
            }
        }
    } else {
        // Catalogo da Comunidade
        int start_idx = std::clamp( mods_scroll_offset, 0, std::max( 0, static_cast<int>( community_mods.size() ) - 1 ) );
        int end_idx = std::min( start_idx + visible_count, static_cast<int>( community_mods.size() ) );

        for( int i = start_idx; i < end_idx; ++i ) {
            auto &mod = community_mods[i];
            int cy_pos = list_y + ( i - start_idx ) * ( card_h + card_gap );

            SDL_Rect card_rect = { settings_box.x + 22, cy_pos, settings_box.w - 44, card_h };
            SetRenderDrawColor( renderer, 22, 28, 38, 220 );
            RenderFillRect( renderer, &card_rect );
            SetRenderDrawColor( renderer, 45, 60, 80, 255 );
            RenderDrawRect( renderer, &card_rect );

            // Linha 1: Nome, Autor, Tamanho
            std::string header_line = mod.name + " (" + mod.version + ") por " + mod.author + "  |  " + mod.disk_size;
            font->OutputChar( renderer, header_line, point( card_rect.x + 12, card_rect.y + 6 ), 11 );

            // Linha 2: Tags
            font->OutputChar( renderer, mod.tags, point( card_rect.x + 12, card_rect.y + 26 ), 10 );

            // Linha 3: Descricao
            std::string desc_line = mod.description;
            if( utf8_width( desc_line ) > 60 ) {
                desc_line = desc_line.substr( 0, 57 ) + "...";
            }
            font->OutputChar( renderer, desc_line, point( card_rect.x + 12, card_rect.y + 46 ), 15 );

            // Botao Instalar / Ja Instalado
            mod.install_rect = { card_rect.x + card_rect.w - 190, card_rect.y + 12, 175, card_h - 24 };
            if( mod.is_installed ) {
                SetRenderDrawColor( renderer, 39, 174, 96, 240 ); // Verde Instalado
            } else {
                SetRenderDrawColor( renderer, 41, 128, 185, 240 ); // Azul Instalar
            }
            RenderFillRect( renderer, &mod.install_rect );
            SetRenderDrawColor( renderer, 100, 200, 255, 255 );
            RenderDrawRect( renderer, &mod.install_rect );

            std::string inst_lbl = mod.is_installed ? _( "[ INSTALADO ]" ) : _( "[ INSTALAR ]" );
            int tx = mod.install_rect.x + ( mod.install_rect.w - ( font->width * utf8_width( inst_lbl ) ) ) / 2;
            int ty = mod.install_rect.y + ( mod.install_rect.h - font->height ) / 2;
            font->OutputChar( renderer, inst_lbl, point( tx, ty ), 15 );
        }
    }

    // Rodape com botoes de rolagem da lista
    int foot_y = list_y + visible_count * ( card_h + card_gap ) + 6;
    mods_btn_scroll_up = { settings_box.x + 22, foot_y, 160, 36 };
    mods_btn_scroll_down = { settings_box.x + settings_box.w - 182, foot_y, 160, 36 };

    SetRenderDrawColor( renderer, 35, 45, 60, 240 );
    RenderFillRect( renderer, &mods_btn_scroll_up );
    RenderFillRect( renderer, &mods_btn_scroll_down );
    SetRenderDrawColor( renderer, 80, 100, 130, 255 );
    RenderDrawRect( renderer, &mods_btn_scroll_up );
    RenderDrawRect( renderer, &mods_btn_scroll_down );

    std::string up_lbl = _( "[ ^ SUBIR ]" );
    int ux = mods_btn_scroll_up.x + ( mods_btn_scroll_up.w - ( font->width * utf8_width( up_lbl ) ) ) / 2;
    int uy = mods_btn_scroll_up.y + ( mods_btn_scroll_up.h - font->height ) / 2;
    font->OutputChar( renderer, up_lbl, point( ux, uy ), 15 );

    std::string down_lbl = _( "[ v DESCER ]" );
    int dx = mods_btn_scroll_down.x + ( mods_btn_scroll_down.w - ( font->width * utf8_width( down_lbl ) ) ) / 2;
    int dy = mods_btn_scroll_down.y + ( mods_btn_scroll_down.h - font->height ) / 2;
    font->OutputChar( renderer, down_lbl, point( dx, dy ), 15 );

    // Indicador de posicao central
    size_t total_items = ( mods_subtab == 0 ) ? installed_mods.size() : community_mods.size();
    std::string pos_str = string_format( _( "Item %d-%d de %d" ),
        total_items == 0 ? 0 : mods_scroll_offset + 1,
        std::min( mods_scroll_offset + visible_count, static_cast<int>( total_items ) ),
        static_cast<int>( total_items ) );
    int px = settings_box.x + ( settings_box.w - ( font->width * utf8_width( pos_str ) ) ) / 2;
    int py = foot_y + ( 36 - font->height ) / 2;
    font->OutputChar( renderer, pos_str, point( px, py ), 11 );
}

bool mobile_ui_system::handle_mods_tab_touch( float x, float y )
{
    // 1. Sub-aba: Meus Mods
    if( x >= mods_btn_my_mods.x && x <= mods_btn_my_mods.x + mods_btn_my_mods.w &&
        y >= mods_btn_my_mods.y && y <= mods_btn_my_mods.y + mods_btn_my_mods.h ) {
        mods_subtab = 0;
        mods_scroll_offset = 0;
        return true;
    }

    // 2. Sub-aba: Repositorio Comunidade
    if( x >= mods_btn_community.x && x <= mods_btn_community.x + mods_btn_community.w &&
        y >= mods_btn_community.y && y <= mods_btn_community.y + mods_btn_community.h ) {
        mods_subtab = 1;
        mods_scroll_offset = 0;
        return true;
    }

    // 3. Botao Importar
    if( x >= mods_btn_import.x && x <= mods_btn_import.x + mods_btn_import.w &&
        y >= mods_btn_import.y && y <= mods_btn_import.y + mods_btn_import.h ) {
        import_mods_from_disk();
        return true;
    }

    // 4. Botoes de rolagem
    if( x >= mods_btn_scroll_up.x && x <= mods_btn_scroll_up.x + mods_btn_scroll_up.w &&
        y >= mods_btn_scroll_up.y && y <= mods_btn_scroll_up.y + mods_btn_scroll_up.h ) {
        if( mods_scroll_offset > 0 ) {
            mods_scroll_offset--;
        }
        return true;
    }
    if( x >= mods_btn_scroll_down.x && x <= mods_btn_scroll_down.x + mods_btn_scroll_down.w &&
        y >= mods_btn_scroll_down.y && y <= mods_btn_scroll_down.y + mods_btn_scroll_down.h ) {
        size_t total_items = ( mods_subtab == 0 ) ? installed_mods.size() : community_mods.size();
        if( mods_scroll_offset + 3 < static_cast<int>( total_items ) ) {
            mods_scroll_offset++;
        }
        return true;
    }

    // 5. Toque nos botoes de cada card
    if( mods_subtab == 0 ) {
        for( size_t i = 0; i < installed_mods.size(); ++i ) {
            const auto &btn = installed_mods[i].toggle_rect;
            if( btn.w > 0 && x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h ) {
                toggle_mod_active_state( i );
                return true;
            }
        }
    } else {
        for( size_t i = 0; i < community_mods.size(); ++i ) {
            const auto &btn = community_mods[i].install_rect;
            if( btn.w > 0 && x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h ) {
                if( !community_mods[i].is_installed ) {
                    install_community_mod( i );
                }
                return true;
            }
        }
    }

    return true;
}


// ============================================================================
// SISTEMA DE ESQUEMAS DE INTERFACE E GATILHOS PERSONALIZADOS
// ============================================================================

void mobile_ui_system::draw_custom_trigger_button( const SDL_Renderer_Ptr &renderer, Font *font, const custom_trigger &trig, bool highlight )
{
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    Uint8 a_bg = static_cast<Uint8>( std::clamp( trig.opacity * 255.0f, 30.0f, 255.0f ) );
    Uint8 a_border = static_cast<Uint8>( std::min( 255.0f, a_bg * 1.25f ) );

    // Paletas de cores tematicas
    Uint8 r_bg = 25, g_bg = 45, b_bg = 80;
    Uint8 r_bd = 50, g_bd = 140, b_bd = 240;

    switch( trig.color_theme ) {
        case 1: // Verde Sobrevivencia
            r_bg = 20; g_bg = 60; b_bg = 35;
            r_bd = 40; g_bd = 190; b_bd = 90;
            break;
        case 2: // Vermelho Alerta / Combate
            r_bg = 75; g_bg = 25; b_bg = 25;
            r_bd = 220; g_bd = 60; b_bd = 50;
            break;
        case 3: // Dourado / Ambar
            r_bg = 70; g_bg = 55; b_bg = 15;
            r_bd = 230; g_bd = 180; b_bd = 40;
            break;
        case 4: // Cinza Aco Metalico
            r_bg = 35; g_bg = 40; b_bg = 48;
            r_bd = 140; g_bd = 150; b_bd = 165;
            break;
        case 5: // Roxo Arcano
            r_bg = 55; g_bg = 25; b_bg = 75;
            r_bd = 180; g_bd = 70; b_bd = 230;
            break;
        case 0: // Azul Tatil (Padrao)
        default:
            r_bg = 25; g_bg = 45; b_bg = 80;
            r_bd = 50; g_bd = 140; b_bd = 240;
            break;
    }

    if( trig.is_pressed ) {
        r_bg = std::min( 255, r_bg + 60 );
        g_bg = std::min( 255, g_bg + 60 );
        b_bg = std::min( 255, b_bg + 60 );
    }

    // Fundo do botao
    SetRenderDrawColor( renderer, r_bg, g_bg, b_bg, a_bg );
    RenderFillRect( renderer, &trig.calculated_bounds );

    // Borda
    SetRenderDrawColor( renderer, r_bd, g_bd, b_bd, a_border );
    RenderDrawRect( renderer, &trig.calculated_bounds );

    // Destaque pulsante no modo de posicionamento
    if( highlight ) {
        SDL_Rect hl_rect = {
            trig.calculated_bounds.x - 3,
            trig.calculated_bounds.y - 3,
            trig.calculated_bounds.w + 6,
            trig.calculated_bounds.h + 6
        };
        SetRenderDrawColor( renderer, 255, 215, 0, 255 ); // Dourado brilhante
        RenderDrawRect( renderer, &hl_rect );
        SDL_Rect hl_rect2 = {
            trig.calculated_bounds.x - 5,
            trig.calculated_bounds.y - 5,
            trig.calculated_bounds.w + 10,
            trig.calculated_bounds.h + 10
        };
        SetRenderDrawColor( renderer, 100, 200, 255, 180 );
        RenderDrawRect( renderer, &hl_rect2 );
    }

    // Rotulo central
    if( font ) {
        std::string lbl = trig.label;
        if( utf8_width( lbl ) > 6 ) {
            lbl = lbl.substr( 0, 5 ) + ".";
        }
        int tx = trig.calculated_bounds.x + ( trig.calculated_bounds.w - ( font->width * utf8_width( lbl ) ) ) / 2;
        int ty = trig.calculated_bounds.y + ( trig.calculated_bounds.h - font->height ) / 2;
        font->OutputChar( renderer, lbl, point( tx, ty ), 15, 1.0f );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::load_all_schemes()
{
    loaded_schemes.clear();

    // 1. Esquema Padrao Oficial
    interface_scheme def_scheme;
    def_scheme.id = "padrao";
    def_scheme.name = _( "Esquema Padrao (Oficial)" );
    def_scheme.description = _( "Layout de fabrica oficial com D-Pad e cluster ergonomico de combate e sobrevivencia." );
    def_scheme.is_default = true;
    loaded_schemes.push_back( def_scheme );

    std::vector<std::string> search_dirs;
    search_dirs.push_back( PATH_INFO::user_dir() + "schemes/" );
    search_dirs.push_back( "/storage/emulated/0/Documents/CataclysmDDA/schemes/" );

    std::set<std::string> seen_scheme_ids;
    seen_scheme_ids.insert( "padrao" );

    for( const auto &dir_path : search_dirs ) {
        if( !dir_exist( dir_path ) ) {
            continue;
        }

        try {
            for( const auto &entry : std::filesystem::directory_iterator( std::filesystem::u8path( dir_path ) ) ) {
                if( !entry.is_regular_file() || entry.path().extension().u8string() != ".json" ) {
                    continue;
                }

                std::string file_path = entry.path().u8string();
                std::string stem = entry.path().stem().u8string();

                if( seen_scheme_ids.count( stem ) > 0 ) {
                    continue;
                }

                interface_scheme s;
                s.id = stem;
                s.name = ( stem == "esquema_atual_temporario" ) ? _( "Temporario (Auto-Salvo)" ) : stem;
                s.description = _( "Esquema customizado de gatilhos tatil." );
                s.is_default = false;

                try {
                    cata_path cp( cata_path::root_path::unknown, file_path );
                    read_from_file_json( cp, [&]( const JsonValue &jv ) {
                        JsonObject jo = jv.get_object();
                        jo.allow_omitted_members();
                        if( jo.has_string( "name" ) ) {
                            s.name = jo.get_string( "name" );
                        }
                        if( jo.has_string( "description" ) ) {
                            s.description = jo.get_string( "description" );
                        }
                        if( jo.has_bool( "show_analog" ) ) {
                            s.show_analog = jo.get_bool( "show_analog" );
                        } else {
                            s.show_analog = true;
                        }
                        if( jo.has_bool( "show_fixed_buttons" ) ) {
                            s.show_fixed_buttons = jo.get_bool( "show_fixed_buttons" );
                        } else {
                            s.show_fixed_buttons = true;
                        }
                        if( jo.has_bool( "show_context_slots" ) ) {
                            s.show_context_slots = jo.get_bool( "show_context_slots" );
                        } else {
                            s.show_context_slots = true;
                        }
                        if( jo.has_array( "triggers" ) ) {
                            JsonArray ja = jo.get_array( "triggers" );
                            for( size_t ti = 0; ti < ja.size(); ++ti ) {
                                JsonObject to = ja.get_object( ti );
                                to.allow_omitted_members();
                                custom_trigger trig;
                                if( to.has_string( "id" ) ) trig.id = to.get_string( "id" );
                                if( to.has_string( "label" ) ) trig.label = to.get_string( "label" );
                                if( to.has_float( "x_pct" ) ) trig.x_pct = to.get_float( "x_pct" );
                                if( to.has_float( "y_pct" ) ) trig.y_pct = to.get_float( "y_pct" );
                                if( to.has_int( "size" ) ) trig.size = to.get_int( "size" );
                                if( to.has_float( "opacity" ) ) trig.opacity = to.get_float( "opacity" );
                                if( to.has_int( "color_theme" ) ) trig.color_theme = to.get_int( "color_theme" );
                                if( to.has_array( "keys" ) ) {
                                    trig.keys = to.get_int_array( "keys" );
                                }
                                s.triggers.push_back( trig );
                            }
                        }
                    } );
                } catch( ... ) {}

                seen_scheme_ids.insert( stem );
                loaded_schemes.push_back( s );
            }
        } catch( ... ) {}
    }

    if( selected_scheme_idx >= static_cast<int>( loaded_schemes.size() ) ) {
        selected_scheme_idx = 0;
    }
}

void mobile_ui_system::save_scheme_to_disk( const interface_scheme &scheme )
{
    std::string internal_dir = PATH_INFO::user_dir() + "schemes/";
    std::string public_dir = "/storage/emulated/0/Documents/CataclysmDDA/schemes/";

    std::error_code ec;
    std::filesystem::create_directories( std::filesystem::u8path( internal_dir ), ec );
    std::filesystem::create_directories( std::filesystem::u8path( public_dir ), ec );

    std::string file_name = scheme.id + ".json";
    std::string file_path = internal_dir + file_name;

    cata_path cp( cata_path::root_path::unknown, file_path );
    write_to_file( cp, [&]( std::ostream &fout ) {
        JsonOut json( fout, true );
        json.start_object();
        json.member( "id", scheme.id );
        json.member( "name", scheme.name );
        json.member( "description", scheme.description );
        json.member( "show_analog", scheme.show_analog );
        json.member( "show_fixed_buttons", scheme.show_fixed_buttons );
        json.member( "show_context_slots", scheme.show_context_slots );
        json.member( "triggers" );
        json.start_array();
        for( const auto &trig : scheme.triggers ) {
            json.start_object();
            json.member( "id", trig.id );
            json.member( "label", trig.label );
            json.member( "x_pct", trig.x_pct );
            json.member( "y_pct", trig.y_pct );
            json.member( "size", trig.size );
            json.member( "opacity", trig.opacity );
            json.member( "color_theme", trig.color_theme );
            json.member( "keys" );
            json.start_array();
            for( int k : trig.keys ) {
                json.write( k );
            }
            json.end_array();
            json.end_object();
        }
        json.end_array();
        json.end_object();
    } );

    // Copia de seguranca para a pasta publica de Documentos
    try {
        std::filesystem::copy_file( std::filesystem::u8path( file_path ),
                                    std::filesystem::u8path( public_dir + file_name ),
                                    std::filesystem::copy_options::overwrite_existing, ec );
    } catch( ... ) {}
}

void mobile_ui_system::save_current_as_temporary()
{
    if( selected_scheme_idx <= 0 || selected_scheme_idx >= static_cast<int>( loaded_schemes.size() ) ) {
        return;
    }

    auto &current = loaded_schemes[selected_scheme_idx];
    interface_scheme temp = current;
    temp.id = "esquema_atual_temporario";
    temp.name = _( "Temporario (Auto-Salvo)" );
    temp.is_default = false;

    save_scheme_to_disk( temp );
}

void mobile_ui_system::delete_scheme_from_disk( size_t index )
{
    if( index == 0 || index >= loaded_schemes.size() ) {
        return; // Nao exclui o esquema padrao
    }

    std::string id = loaded_schemes[index].id;
    std::string internal_file = PATH_INFO::user_dir() + "schemes/" + id + ".json";
    std::string public_file = "/storage/emulated/0/Documents/CataclysmDDA/schemes/" + id + ".json";

    std::error_code ec;
    std::filesystem::remove( std::filesystem::u8path( internal_file ), ec );
    std::filesystem::remove( std::filesystem::u8path( public_file ), ec );

    if( active_applied_scheme_idx == static_cast<int>( index ) ) {
        active_applied_scheme_idx = 0; // Volta para o padrao se excluiu o ativo
    }

    load_all_schemes();
    selected_scheme_idx = 0;
    schemes_feedback_msg = _( "Esquema excluido com sucesso." );
}

void mobile_ui_system::apply_scheme( size_t index )
{
    if( index >= loaded_schemes.size() ) {
        return;
    }

    active_applied_scheme_idx = static_cast<int>( index );
    schemes_feedback_msg = string_format( _( "Esquema '%s' ativado no jogo!" ), loaded_schemes[index].name );
}

// ============================================================================
// DESENHO DA ABA DE ATALHOS & ESQUEMAS (DESIGN ESTILO CIVILOPEDIA / 2 COLUNAS)
// ============================================================================

void mobile_ui_system::draw_schemes_tab( const SDL_Renderer_Ptr &renderer, Font *font )
{
    int tab_y = 66;
    int tab_h = 48;
    int content_y = tab_y + tab_h + 16;
    int content_h = screen_height - content_y - 20;

    // ========================================================================
    // COLUNA ESQUERDA: Lista de Esquemas (30% da tela, min 320px)
    // ========================================================================
    int col_left_w = std::max( 320, static_cast<int>( screen_width * 0.28f ) );
    int col_left_x = 24;
    SDL_Rect col_left = { col_left_x, content_y, col_left_w, content_h };

    SetRenderDrawColor( renderer, 12, 18, 30, 240 );
    RenderFillRect( renderer, &col_left );
    SetRenderDrawColor( renderer, 28, 44, 70, 255 );
    RenderDrawRect( renderer, &col_left );

    // Botao no topo da coluna esquerda: [ + NOVO ESQUEMA ]
    schemes_btn_new_scheme = { col_left.x + 10, col_left.y + 10, col_left.w - 20, 46 };
    SetRenderDrawColor( renderer, 22, 100, 160, 240 );
    RenderFillRect( renderer, &schemes_btn_new_scheme );
    SetRenderDrawColor( renderer, 60, 200, 255, 255 );
    RenderDrawRect( renderer, &schemes_btn_new_scheme );

    std::string new_sch_lbl = _( "+ NOVO ESQUEMA" );
    int ns_x = schemes_btn_new_scheme.x + ( schemes_btn_new_scheme.w - ( font->width * utf8_width( new_sch_lbl ) ) ) / 2;
    int ns_y = schemes_btn_new_scheme.y + ( schemes_btn_new_scheme.h - font->height ) / 2;
    font->OutputChar( renderer, new_sch_lbl, point( ns_x, ns_y ), 11 );

    // Lista de cards de esquemas (Cards estilo Civilopedia com circulo branco a esquerda)
    int card_y = schemes_btn_new_scheme.y + schemes_btn_new_scheme.h + 10;
    int card_h = 52;
    int card_gap = 8;
    int max_visible_cards = ( col_left.y + col_left.h - card_y - 46 ) / ( card_h + card_gap );

    schemes_cards_rects.clear();
    int start_s_idx = std::clamp( scheme_list_scroll_offset, 0, std::max( 0, static_cast<int>( loaded_schemes.size() ) - 1 ) );
    int end_s_idx = std::min( start_s_idx + std::max( 1, max_visible_cards ), static_cast<int>( loaded_schemes.size() ) );

    for( int i = start_s_idx; i < end_s_idx; ++i ) {
        int cy = card_y + ( i - start_s_idx ) * ( card_h + card_gap );
        SDL_Rect card_rect = { col_left.x + 10, cy, col_left.w - 20, card_h };
        schemes_cards_rects.push_back( card_rect );

        bool is_selected = ( i == selected_scheme_idx );
        bool is_active = ( i == active_applied_scheme_idx );

        // Fundo do card (Azul Royal eletrico se selecionado, como o Lancer no print)
        if( is_selected ) {
            SetRenderDrawColor( renderer, 25, 60, 155, 255 ); // Azul Royal Vibrante
            RenderFillRect( renderer, &card_rect );
            SetRenderDrawColor( renderer, 90, 180, 255, 255 ); // Borda luminosa
            RenderDrawRect( renderer, &card_rect );
        } else {
            SetRenderDrawColor( renderer, 18, 28, 44, 210 );
            RenderFillRect( renderer, &card_rect );
            SetRenderDrawColor( renderer, 36, 52, 78, 255 );
            RenderDrawRect( renderer, &card_rect );
        }

        // Circulo branco a esquerda do card (fiel a imagem)
        SDL_Rect icon_circle = { card_rect.x + 8, card_rect.y + ( card_h - 30 ) / 2, 30, 30 };
        SetRenderDrawColor( renderer, 240, 245, 255, 255 );
        RenderFillRect( renderer, &icon_circle );
        SetRenderDrawColor( renderer, 40, 80, 160, 255 );
        RenderDrawRect( renderer, &icon_circle );

        // Letra inicial no circulo
        std::string init_letter = loaded_schemes[i].name.empty() ? "?" : loaded_schemes[i].name.substr( 0, 1 );
        int il_x = icon_circle.x + ( icon_circle.w - font->width ) / 2;
        int il_y = icon_circle.y + ( icon_circle.h - font->height ) / 2;
        font->OutputChar( renderer, init_letter, point( il_x, il_y ), 0 );

        // Nome do esquema
        std::string s_name = loaded_schemes[i].name;
        if( utf8_width( s_name ) > 18 ) {
            s_name = s_name.substr( 0, 17 ) + "..";
        }
        int sn_x = card_rect.x + 46;
        int sn_y = card_rect.y + 8;
        font->OutputChar( renderer, s_name, point( sn_x, sn_y ), is_selected ? 11 : 15 );

        // Tag [ATIVO] se estiver em uso no jogo
        if( is_active ) {
            font->OutputChar( renderer, _( "[ATIVO NO JOGO]" ), point( sn_x, card_rect.y + 28 ), 10 );
        } else {
            std::string trig_cnt = string_format( _( "%d gatilhos" ), static_cast<int>( loaded_schemes[i].triggers.size() ) );
            font->OutputChar( renderer, trig_cnt, point( sn_x, card_rect.y + 28 ), 14 );
        }
    }

    // Botoes de rolagem da lista de esquemas
    int foot_y = col_left.y + col_left.h - 38;
    schemes_btn_scroll_up = { col_left.x + 10, foot_y, ( col_left.w - 26 ) / 2, 30 };
    schemes_btn_scroll_down = { schemes_btn_scroll_up.x + schemes_btn_scroll_up.w + 6, foot_y, ( col_left.w - 26 ) / 2, 30 };

    SetRenderDrawColor( renderer, 30, 44, 65, 220 );
    RenderFillRect( renderer, &schemes_btn_scroll_up );
    RenderFillRect( renderer, &schemes_btn_scroll_down );
    SetRenderDrawColor( renderer, 60, 90, 130, 255 );
    RenderDrawRect( renderer, &schemes_btn_scroll_up );
    RenderDrawRect( renderer, &schemes_btn_scroll_down );

    font->OutputChar( renderer, "^", point( schemes_btn_scroll_up.x + ( schemes_btn_scroll_up.w - font->width ) / 2, foot_y + 8 ), 15 );
    font->OutputChar( renderer, "v", point( schemes_btn_scroll_down.x + ( schemes_btn_scroll_down.w - font->width ) / 2, foot_y + 8 ), 15 );

    // ========================================================================
    // COLUNA DIREITA: Painel Detalhado (Inspirado no print Lancer em Fullscreen)
    // ========================================================================
    int col_right_x = col_left.x + col_left.w + 18;
    int col_right_w = screen_width - col_right_x - 24;
    SDL_Rect col_right = { col_right_x, content_y, col_right_w, content_h };

    SetRenderDrawColor( renderer, 12, 17, 30, 235 );
    RenderFillRect( renderer, &col_right );
    SetRenderDrawColor( renderer, 26, 42, 68, 255 );
    RenderDrawRect( renderer, &col_right );

    if( selected_scheme_idx < 0 || selected_scheme_idx >= static_cast<int>( loaded_schemes.size() ) ) {
        return;
    }

    auto &cur_scheme = loaded_schemes[selected_scheme_idx];

    // Cabecalho do Painel: Circulo branco grande + Titulo destacado
    int head_y = col_right.y + 14;
    SDL_Rect big_circle = { col_right.x + 18, head_y, 42, 42 };
    SetRenderDrawColor( renderer, 240, 245, 255, 255 );
    RenderFillRect( renderer, &big_circle );
    SetRenderDrawColor( renderer, 60, 120, 220, 255 );
    RenderDrawRect( renderer, &big_circle );

    font->OutputChar( renderer, "*", point( big_circle.x + ( big_circle.w - font->width ) / 2, big_circle.y + ( big_circle.h - font->height ) / 2 ), 0 );

    // Titulo do Esquema
    int title_x = big_circle.x + big_circle.w + 14;
    std::string sch_title = cur_scheme.name;
    if( cur_scheme.is_default ) {
        sch_title += _( "  [PADRÃO DE FÁBRICA - SOMENTE LEITURA]" );
    }
    font->OutputChar( renderer, sch_title, point( title_x, head_y + 12 ), 11 );

    // Linha separadora horizontal estilizada dourada (como no print!)
    SDL_Rect sep_line = { col_right.x + 18, head_y + 50, col_right.w - 36, 2 };
    SetRenderDrawColor( renderer, 195, 165, 80, 230 ); // Dourado nobre
    RenderFillRect( renderer, &sep_line );

    // Informacoes e Status em topicos limpos
    int info_y = sep_line.y + 12;
    std::string desc_str = cur_scheme.description;
    font->OutputChar( renderer, desc_str, point( col_right.x + 18, info_y ), 15 );

    std::string stat_str = string_format( _( "Gatilhos: %d  |  Analógico: %s  |  Botões Base: %s  |  Status: %s" ),
        static_cast<int>( cur_scheme.triggers.size() ),
        cur_scheme.show_analog ? _( "ATIVO" ) : _( "LIMPO" ),
        cur_scheme.show_fixed_buttons ? _( "ATIVO" ) : _( "LIMPO (SÓ HUB)" ),
        ( selected_scheme_idx == active_applied_scheme_idx ) ? _( "[ATIVO NO JOGO]" ) : _( "[NÃO APLICADO]" ) );
    font->OutputChar( renderer, stat_str, point( col_right.x + 18, info_y + 24 ), 14 );

    // Mensagem de feedback se houver
    if( !schemes_feedback_msg.empty() ) {
        font->OutputChar( renderer, schemes_feedback_msg, point( col_right.x + 18, info_y + 46 ), 10 );
    }

    // ========================================================================
    // Barra de Botoes de Acao (6 BOTOES AMPLOS E ESPACADOS EM FULLSCREEN)
    // [ APLICAR ], [ + GATILHO ], [ POSICIONAR ], [ SALVAR ], [ LIMPAR ], [ EXCLUIR ]
    // ========================================================================
    int actions_y = info_y + 68;
    int btn_h = 44;
    int act_gap = 10;
    int total_btns_w = col_right.w - 36;
    int btn_w = ( total_btns_w - ( act_gap * 5 ) ) / 6;

    scheme_btn_apply = { col_right.x + 18 + 0 * ( btn_w + act_gap ), actions_y, btn_w, btn_h };
    scheme_btn_add_trigger = { col_right.x + 18 + 1 * ( btn_w + act_gap ), actions_y, btn_w, btn_h };
    scheme_btn_position_screen = { col_right.x + 18 + 2 * ( btn_w + act_gap ), actions_y, btn_w, btn_h };
    scheme_btn_save_as = { col_right.x + 18 + 3 * ( btn_w + act_gap ), actions_y, btn_w, btn_h };
    scheme_btn_clear_scheme = { col_right.x + 18 + 4 * ( btn_w + act_gap ), actions_y, btn_w, btn_h };
    scheme_btn_delete = { col_right.x + 18 + 5 * ( btn_w + act_gap ), actions_y, btn_w, btn_h };

    // Botao 1: [ APLICAR ]
    bool is_cur_active = ( selected_scheme_idx == active_applied_scheme_idx );
    SetRenderDrawColor( renderer, is_cur_active ? 20 : 35, is_cur_active ? 130 : 75, is_cur_active ? 70 : 170, 240 );
    RenderFillRect( renderer, &scheme_btn_apply );
    SetRenderDrawColor( renderer, 70, 190, 255, 255 );
    RenderDrawRect( renderer, &scheme_btn_apply );
    std::string app_lbl = is_cur_active ? _( "EM USO" ) : _( "APLICAR" );
    int ax = scheme_btn_apply.x + ( scheme_btn_apply.w - ( font->width * utf8_width( app_lbl ) ) ) / 2;
    int ay = scheme_btn_apply.y + ( scheme_btn_apply.h - font->height ) / 2;
    font->OutputChar( renderer, app_lbl, point( ax, ay ), 15 );

    // Botao 2: [ + GATILHO ] (Desabilitado se padrao)
    bool can_edit_cur = !cur_scheme.is_default;
    SetRenderDrawColor( renderer, can_edit_cur ? 20 : 35, can_edit_cur ? 100 : 40, can_edit_cur ? 160 : 50, can_edit_cur ? 240 : 120 );
    RenderFillRect( renderer, &scheme_btn_add_trigger );
    SetRenderDrawColor( renderer, can_edit_cur ? 50 : 70, can_edit_cur ? 200 : 70, can_edit_cur ? 255 : 80, 255 );
    RenderDrawRect( renderer, &scheme_btn_add_trigger );
    std::string add_lbl = can_edit_cur ? _( "+ GATILHO" ) : _( "+ GATILHO (BLOQ)" );
    int gx = scheme_btn_add_trigger.x + ( scheme_btn_add_trigger.w - ( font->width * utf8_width( add_lbl ) ) ) / 2;
    int gy = scheme_btn_add_trigger.y + ( scheme_btn_add_trigger.h - font->height ) / 2;
    font->OutputChar( renderer, add_lbl, point( gx, gy ), can_edit_cur ? 11 : 14 );

    // Botao 3: [ POSICIONAR ] (Desabilitado se padrao)
    SetRenderDrawColor( renderer, can_edit_cur ? 160 : 40, can_edit_cur ? 110 : 40, can_edit_cur ? 25 : 40, can_edit_cur ? 240 : 120 );
    RenderFillRect( renderer, &scheme_btn_position_screen );
    SetRenderDrawColor( renderer, can_edit_cur ? 255 : 70, can_edit_cur ? 200 : 70, can_edit_cur ? 50 : 70, 255 );
    RenderDrawRect( renderer, &scheme_btn_position_screen );
    std::string pos_lbl = can_edit_cur ? _( "POSICIONAR" ) : _( "POSICIONAR (BLOQ)" );
    int pox = scheme_btn_position_screen.x + ( scheme_btn_position_screen.w - ( font->width * utf8_width( pos_lbl ) ) ) / 2;
    int poy = scheme_btn_position_screen.y + ( scheme_btn_position_screen.h - font->height ) / 2;
    font->OutputChar( renderer, pos_lbl, point( pox, poy ), can_edit_cur ? 11 : 14 );

    // Botao 4: [ SALVAR COMO ]
    SetRenderDrawColor( renderer, 45, 65, 95, 240 );
    RenderFillRect( renderer, &scheme_btn_save_as );
    SetRenderDrawColor( renderer, 100, 150, 210, 255 );
    RenderDrawRect( renderer, &scheme_btn_save_as );
    std::string sav_lbl = _( "SALVAR" );
    int sx = scheme_btn_save_as.x + ( scheme_btn_save_as.w - ( font->width * utf8_width( sav_lbl ) ) ) / 2;
    int sy = scheme_btn_save_as.y + ( scheme_btn_save_as.h - font->height ) / 2;
    font->OutputChar( renderer, sav_lbl, point( sx, sy ), 15 );

    // Botao 5: [ LIMPAR ] (Deleta todos os botoes, mantem apenas o HUB em tela 100% do tempo)
    SetRenderDrawColor( renderer, can_edit_cur ? 175 : 40, can_edit_cur ? 75 : 40, can_edit_cur ? 20 : 40, can_edit_cur ? 240 : 120 );
    RenderFillRect( renderer, &scheme_btn_clear_scheme );
    SetRenderDrawColor( renderer, can_edit_cur ? 255 : 70, can_edit_cur ? 130 : 70, can_edit_cur ? 40 : 70, 255 );
    RenderDrawRect( renderer, &scheme_btn_clear_scheme );
    std::string clr_lbl = can_edit_cur ? _( "LIMPAR" ) : _( "LIMPAR (BLOQ)" );
    int clx = scheme_btn_clear_scheme.x + ( scheme_btn_clear_scheme.w - ( font->width * utf8_width( clr_lbl ) ) ) / 2;
    int cly = scheme_btn_clear_scheme.y + ( scheme_btn_clear_scheme.h - font->height ) / 2;
    font->OutputChar( renderer, clr_lbl, point( clx, cly ), can_edit_cur ? 15 : 14 );

    // Botao 6: [ EXCLUIR ] (Desabilitado se padrao)
    bool can_delete = !cur_scheme.is_default;
    SetRenderDrawColor( renderer, can_delete ? 150 : 40, can_delete ? 35 : 40, can_delete ? 35 : 40, can_delete ? 240 : 120 );
    RenderFillRect( renderer, &scheme_btn_delete );
    SetRenderDrawColor( renderer, can_delete ? 230 : 70, can_delete ? 70 : 70, can_delete ? 70 : 70, 255 );
    RenderDrawRect( renderer, &scheme_btn_delete );
    std::string del_lbl = can_delete ? _( "EXCLUIR" ) : _( "EXCLUIR (BLOQ)" );
    int dx = scheme_btn_delete.x + ( scheme_btn_delete.w - ( font->width * utf8_width( del_lbl ) ) ) / 2;
    int dy = scheme_btn_delete.y + ( scheme_btn_delete.h - font->height ) / 2;
    font->OutputChar( renderer, del_lbl, point( dx, dy ), can_delete ? 15 : 14 );

    // ========================================================================
    // Botoes Toggles Rapidos: [ ANALOGICO: ON/OFF ] e [ BOTOES BASE: ON/OFF ]
    // ========================================================================
    int toggles_y = actions_y + btn_h + 10;
    scheme_btn_toggle_analog = { col_right.x + 18, toggles_y, 230, 32 };
    scheme_btn_toggle_fixed = { scheme_btn_toggle_analog.x + scheme_btn_toggle_analog.w + 12, toggles_y, 250, 32 };

    if( can_edit_cur ) {
        // Toggle Analogico
        SetRenderDrawColor( renderer, cur_scheme.show_analog ? 30 : 50, cur_scheme.show_analog ? 100 : 30, cur_scheme.show_analog ? 50 : 30, 220 );
        RenderFillRect( renderer, &scheme_btn_toggle_analog );
        SetRenderDrawColor( renderer, cur_scheme.show_analog ? 60 : 100, cur_scheme.show_analog ? 200 : 60, cur_scheme.show_analog ? 100 : 60, 255 );
        RenderDrawRect( renderer, &scheme_btn_toggle_analog );
        std::string ana_lbl = cur_scheme.show_analog ? _( "[ ANALÓGICO: ATIVADO ]" ) : _( "[ ANALÓGICO: LIMPO ]" );
        font->OutputChar( renderer, ana_lbl, point( scheme_btn_toggle_analog.x + 10, scheme_btn_toggle_analog.y + 8 ), 15 );

        // Toggle Botoes Base
        SetRenderDrawColor( renderer, cur_scheme.show_fixed_buttons ? 30 : 50, cur_scheme.show_fixed_buttons ? 100 : 30, cur_scheme.show_fixed_buttons ? 50 : 30, 220 );
        RenderFillRect( renderer, &scheme_btn_toggle_fixed );
        SetRenderDrawColor( renderer, cur_scheme.show_fixed_buttons ? 60 : 100, cur_scheme.show_fixed_buttons ? 200 : 60, cur_scheme.show_fixed_buttons ? 100 : 60, 255 );
        RenderDrawRect( renderer, &scheme_btn_toggle_fixed );
        std::string fix_lbl = cur_scheme.show_fixed_buttons ? _( "[ BOTÕES BASE: ATIVOS ]" ) : _( "[ BOTÕES BASE: LIMPOS ]" );
        font->OutputChar( renderer, fix_lbl, point( scheme_btn_toggle_fixed.x + 10, scheme_btn_toggle_fixed.y + 8 ), 15 );
    }

    // ========================================================================
    // Lista de Gatilhos Configurados deste Esquema
    // ========================================================================
    int t_list_y = toggles_y + 42;
    int t_card_h = 48;
    int t_gap = 8;
    int visible_t_count = ( col_right.y + col_right.h - t_list_y - 10 ) / ( t_card_h + t_gap );

    trigger_item_delete_rects.clear();

    if( cur_scheme.triggers.empty() ) {
        font->OutputChar( renderer, _( "Nenhum gatilho customizado neste esquema." ), point( col_right.x + 24, t_list_y + 16 ), 14 );
        font->OutputChar( renderer, _( "Toque em [+ GATILHO] acima para adicionar novos botoes personalizados!" ), point( col_right.x + 24, t_list_y + 40 ), 11 );
    } else {
        int start_t_idx = std::clamp( trigger_list_scroll_offset, 0, std::max( 0, static_cast<int>( cur_scheme.triggers.size() ) - 1 ) );
        int end_t_idx = std::min( start_t_idx + std::max( 1, visible_t_count ), static_cast<int>( cur_scheme.triggers.size() ) );

        for( int i = start_t_idx; i < end_t_idx; ++i ) {
            auto &trig = cur_scheme.triggers[i];
            int ty_pos = t_list_y + ( i - start_t_idx ) * ( t_card_h + t_gap );

            SDL_Rect t_card = { col_right.x + 18, ty_pos, col_right.w - 36, t_card_h };
            SetRenderDrawColor( renderer, 20, 28, 44, 220 );
            RenderFillRect( renderer, &t_card );
            SetRenderDrawColor( renderer, 40, 60, 90, 255 );
            RenderDrawRect( renderer, &t_card );

            // Miniatura visual do botao com sua cor real
            SDL_Rect mini_box = { t_card.x + 8, t_card.y + 6, 52, t_card_h - 12 };
            Uint8 r_bg = 25, g_bg = 45, b_bg = 80;
            if( trig.color_theme == 1 ) { r_bg = 20; g_bg = 60; b_bg = 35; }
            else if( trig.color_theme == 2 ) { r_bg = 75; g_bg = 25; b_bg = 25; }
            else if( trig.color_theme == 3 ) { r_bg = 70; g_bg = 55; b_bg = 15; }
            else if( trig.color_theme == 4 ) { r_bg = 35; g_bg = 40; b_bg = 48; }
            else if( trig.color_theme == 5 ) { r_bg = 55; g_bg = 25; b_bg = 75; }

            SetRenderDrawColor( renderer, r_bg, g_bg, b_bg, 255 );
            RenderFillRect( renderer, &mini_box );
            SetRenderDrawColor( renderer, 180, 200, 230, 255 );
            RenderDrawRect( renderer, &mini_box );

            font->OutputChar( renderer, trig.label.substr( 0, 5 ), point( mini_box.x + 4, mini_box.y + 8 ), 15 );

            // Descricao das teclas e posicoes
            std::string keys_str = _( "Teclas: " );
            for( size_t k = 0; k < trig.keys.size(); ++k ) {
                if( k > 0 ) keys_str += " + ";
                int key_val = trig.keys[k];
                if( key_val >= 32 && key_val <= 126 ) {
                    keys_str += std::string( 1, static_cast<char>( key_val ) );
                } else if( key_val == '\n' || key_val == KEY_ENTER ) {
                    keys_str += "ENTER";
                } else if( key_val == 27 ) {
                    keys_str += "ESC";
                } else {
                    keys_str += string_format( "#%d", key_val );
                }
            }

            int text_x = mini_box.x + mini_box.w + 14;
            font->OutputChar( renderer, keys_str, point( text_x, t_card.y + 6 ), 11 );

            std::string geom_str = string_format( _( "Posicao: (%d%%, %d%%)  |  Tamanho: %dpx  |  Opacidade: %d%%" ),
                static_cast<int>( trig.x_pct * 100.0f ),
                static_cast<int>( trig.y_pct * 100.0f ),
                trig.size,
                static_cast<int>( trig.opacity * 100.0f ) );
            font->OutputChar( renderer, geom_str, point( text_x, t_card.y + 26 ), 14 );

            // Botao [ X ] de excluir gatilho (se nao for o padrao)
            if( can_edit_cur ) {
                SDL_Rect del_btn = { t_card.x + t_card.w - 42, t_card.y + 6, 34, t_card_h - 12 };
                trigger_item_delete_rects.push_back( del_btn );

                SetRenderDrawColor( renderer, 150, 35, 35, 230 );
                RenderFillRect( renderer, &del_btn );
                SetRenderDrawColor( renderer, 220, 70, 70, 255 );
                RenderDrawRect( renderer, &del_btn );
                font->OutputChar( renderer, "X", point( del_btn.x + 12, del_btn.y + 8 ), 15 );
            }
        }
    }
}
// MODAL DE CONFIGURACAO DE GATILHO (CATALOGO COMPLETO DE 120 TECLAS)
// ============================================================================

void mobile_ui_system::draw_trigger_config_modal( const SDL_Renderer_Ptr &renderer, Font *font )
{
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    // Fundo escuro modal de sobreposicao
    SDL_Rect overlay = { 0, 0, screen_width, screen_height };
    SetRenderDrawColor( renderer, 5, 8, 15, 215 );
    RenderFillRect( renderer, &overlay );

    // Caixa central
    int m_w = std::min( 920, static_cast<int>( screen_width * 0.94f ) );
    int m_h = std::min( 620, static_cast<int>( screen_height * 0.94f ) );
    SDL_Rect modal_box = { ( screen_width - m_w ) / 2, ( screen_height - m_h ) / 2, m_w, m_h };

    SetRenderDrawColor( renderer, 12, 18, 30, 255 );
    RenderFillRect( renderer, &modal_box );
    SetRenderDrawColor( renderer, 40, 120, 220, 255 );
    RenderDrawRect( renderer, &modal_box );

    // Titulo
    int ty = modal_box.y + 14;
    std::string m_title = _( "CONFIGURAR GATILHO TATIL (TECLADO COMPLETO & FUNCOES)" );
    font->OutputChar( renderer, m_title, point( modal_box.x + 22, ty ), 11 );

    // Previa em tempo real
    temp_editing_trigger.calculated_bounds = { modal_box.x + modal_box.w - 110, modal_box.y + 14, 80, 80 };
    draw_custom_trigger_button( renderer, font, temp_editing_trigger, true );

    // Resumo das teclas ja atribuidas
    std::string assigned_str = _( "Funcoes / Teclas Atribuidas: " );
    if( temp_editing_trigger.keys.empty() ) {
        assigned_str += _( "[ Nenhuma tecla atribuida - Toque em uma tecla abaixo ]" );
    } else {
        for( size_t i = 0; i < temp_editing_trigger.keys.size(); ++i ) {
            if( i > 0 ) assigned_str += " + ";
            int k = temp_editing_trigger.keys[i];
            if( k >= 32 && k <= 126 ) {
                assigned_str += std::string( "'" ) + static_cast<char>( k ) + "'";
            } else if( k == '\n' || k == KEY_ENTER ) {
                assigned_str += "ENTER";
            } else if( k == 27 ) {
                assigned_str += "ESC";
            } else {
                assigned_str += string_format( "K#%d", k );
            }
        }
    }
    font->OutputChar( renderer, assigned_str, point( modal_box.x + 22, ty + 30 ), 15 );

    // Rotulos rapidos
    int lbl_y = ty + 56;
    font->OutputChar( renderer, _( "Rotulo Rapido:" ), point( modal_box.x + 22, lbl_y + 4 ), 14 );
    std::vector<std::string> quick_labels = { "TIRO", "INV", "RECARGA", "OLHAR", "MAPA", "ESPERAR", "ENTER", "ESC", "C1", "C2" };
    int qx = modal_box.x + 130;
    for( const auto &ql : quick_labels ) {
        SDL_Rect q_btn = { qx, lbl_y, static_cast<int>( font->width * utf8_width( ql ) + 14 ), 26 };
        SetRenderDrawColor( renderer, 25, 40, 65, 240 );
        RenderFillRect( renderer, &q_btn );
        SetRenderDrawColor( renderer, 60, 95, 145, 255 );
        RenderDrawRect( renderer, &q_btn );
        font->OutputChar( renderer, ql, point( q_btn.x + 7, q_btn.y + 4 ), 15 );
        qx += q_btn.w + 6;
    }

    // Abas de categorias do teclado completo
    int cat_y = lbl_y + 34;
    std::array<std::string, 6> cat_names = {
        _( "[ a-z ]" ),
        _( "[ A-Z ]" ),
        _( "[ 0-9 ]" ),
        _( "[ SIMBOLOS ]" ),
        _( "[ SISTEMA & SETAS ]" ),
        _( "[ F1-F12 ]" )
    };

    int cat_w = ( modal_box.w - 44 - 30 ) / 6;
    for( int i = 0; i < 6; ++i ) {
        SDL_Rect cat_btn = { modal_box.x + 22 + i * ( cat_w + 6 ), cat_y, cat_w, 32 };
        bool is_c_active = ( trigger_keyboard_category == i );
        SetRenderDrawColor( renderer, is_c_active ? 25 : 18, is_c_active ? 70 : 30, is_c_active ? 130 : 50, 240 );
        RenderFillRect( renderer, &cat_btn );
        SetRenderDrawColor( renderer, is_c_active ? 60 : 40, is_c_active ? 160 : 70, is_c_active ? 255 : 100, 255 );
        RenderDrawRect( renderer, &cat_btn );

        int tx = cat_btn.x + ( cat_btn.w - ( font->width * utf8_width( cat_names[i] ) ) ) / 2;
        int tky = cat_btn.y + ( cat_btn.h - font->height ) / 2;
        font->OutputChar( renderer, cat_names[i], point( tx, tky ), is_c_active ? 11 : 15 );
    }

    // Grid de teclas da categoria selecionada
    int grid_y = cat_y + 40;
    std::vector<std::pair<int, std::string>> current_keys;

    if( trigger_keyboard_category == 0 ) {
        // a-z
        for( char c = 'a'; c <= 'z'; ++c ) {
            current_keys.push_back( { static_cast<int>( c ), std::string( 1, c ) } );
        }
    } else if( trigger_keyboard_category == 1 ) {
        // A-Z
        for( char c = 'A'; c <= 'Z'; ++c ) {
            current_keys.push_back( { static_cast<int>( c ), std::string( 1, c ) } );
        }
    } else if( trigger_keyboard_category == 2 ) {
        // 0-9
        for( char c = '0'; c <= '9'; ++c ) {
            current_keys.push_back( { static_cast<int>( c ), std::string( 1, c ) } );
        }
    } else if( trigger_keyboard_category == 3 ) {
        // Simbolos
        std::string syms = "!@#$%^&*()_+-=[]{}|;':\",./<>?~`";
        for( char c : syms ) {
            current_keys.push_back( { static_cast<int>( c ), std::string( 1, c ) } );
        }
    } else if( trigger_keyboard_category == 4 ) {
        // Sistema & Setas
        current_keys.push_back( { '\n', "ENTER" } );
        current_keys.push_back( { 27, "ESC" } );
        current_keys.push_back( { ' ', "ESPACO" } );
        current_keys.push_back( { '\t', "TAB" } );
        current_keys.push_back( { 127, "BACKSPACE" } );
        current_keys.push_back( { KEY_DC, "DELETE" } );
        current_keys.push_back( { KEY_UP, "^ CIMA" } );
        current_keys.push_back( { KEY_DOWN, "v BAIXO" } );
        current_keys.push_back( { KEY_LEFT, "< ESQ" } );
        current_keys.push_back( { KEY_RIGHT, "> DIR" } );
        current_keys.push_back( { KEY_PPAGE, "PG UP" } );
        current_keys.push_back( { KEY_NPAGE, "PG DN" } );
        current_keys.push_back( { KEY_HOME, "HOME" } );
        current_keys.push_back( { KEY_END, "END" } );
    } else if( trigger_keyboard_category == 5 ) {
        // F1-F12
        for( int f = 1; f <= 12; ++f ) {
            current_keys.push_back( { KEY_F( f ), string_format( "F%d", f ) } );
        }
    }

    int k_btn_w = 54;
    int k_btn_h = 36;
    int k_gap = 6;
    int cols_per_row = ( modal_box.w - 44 ) / ( k_btn_w + k_gap );

    for( size_t i = 0; i < current_keys.size(); ++i ) {
        int r = static_cast<int>( i ) / cols_per_row;
        int c = static_cast<int>( i ) % cols_per_row;

        int kx = modal_box.x + 22 + c * ( k_btn_w + k_gap );
        int ky = grid_y + r * ( k_btn_h + k_gap );

        SDL_Rect k_rect = { kx, ky, k_btn_w, k_btn_h };
        SetRenderDrawColor( renderer, 25, 38, 58, 240 );
        RenderFillRect( renderer, &k_rect );
        SetRenderDrawColor( renderer, 55, 85, 125, 255 );
        RenderDrawRect( renderer, &k_rect );

        std::string d_str = current_keys[i].second;
        int tx = k_rect.x + ( k_rect.w - ( font->width * utf8_width( d_str ) ) ) / 2;
        int t_y = k_rect.y + ( k_rect.h - font->height ) / 2;
        font->OutputChar( renderer, d_str, point( tx, t_y ), 15 );
    }

    // Rodape com Controles de Estilo (Tamanho, Opacidade, Cor) e Botoes de Acao
    int foot_y = modal_box.y + modal_box.h - 90;

    // Linha de ajustes rapidos
    std::string sz_str = string_format( _( "Tamanho: %dpx [ - ] [ + ]" ), temp_editing_trigger.size );
    font->OutputChar( renderer, sz_str, point( modal_box.x + 22, foot_y + 8 ), 14 );

    std::string op_str = string_format( _( "Opacidade: %d%% [ - ] [ + ]" ), static_cast<int>( temp_editing_trigger.opacity * 100.0f ) );
    font->OutputChar( renderer, op_str, point( modal_box.x + 280, foot_y + 8 ), 14 );

    std::string cor_str = string_format( _( "Tema de Cor: %d [ ALTERNAR ]" ), temp_editing_trigger.color_theme + 1 );
    font->OutputChar( renderer, cor_str, point( modal_box.x + 540, foot_y + 8 ), 11 );

    // Botoes finais: [ SALVAR GATILHO ], [ LIMPAR TECLAS ], [ CANCELAR ]
    int act_y = foot_y + 40;
    SDL_Rect btn_save_trig = { modal_box.x + 22, act_y, 240, 38 };
    SetRenderDrawColor( renderer, 35, 150, 85, 255 );
    RenderFillRect( renderer, &btn_save_trig );
    SetRenderDrawColor( renderer, 50, 220, 120, 255 );
    RenderDrawRect( renderer, &btn_save_trig );
    std::string st_lbl = _( "[ SALVAR GATILHO ]" );
    font->OutputChar( renderer, st_lbl, point( btn_save_trig.x + 36, btn_save_trig.y + 10 ), 15 );

    SDL_Rect btn_clear_keys = { btn_save_trig.x + btn_save_trig.w + 14, act_y, 180, 38 };
    SetRenderDrawColor( renderer, 120, 80, 25, 240 );
    RenderFillRect( renderer, &btn_clear_keys );
    SetRenderDrawColor( renderer, 200, 140, 45, 255 );
    RenderDrawRect( renderer, &btn_clear_keys );
    std::string cl_lbl = _( "LIMPAR TECLAS" );
    font->OutputChar( renderer, cl_lbl, point( btn_clear_keys.x + 24, btn_clear_keys.y + 10 ), 15 );

    SDL_Rect btn_cancel_trig = { btn_clear_keys.x + btn_clear_keys.w + 14, act_y, 140, 38 };
    SetRenderDrawColor( renderer, 60, 25, 25, 240 );
    RenderFillRect( renderer, &btn_cancel_trig );
    SetRenderDrawColor( renderer, 150, 50, 50, 255 );
    RenderDrawRect( renderer, &btn_cancel_trig );
    std::string ct_lbl = _( "CANCELAR" );
    font->OutputChar( renderer, ct_lbl, point( btn_cancel_trig.x + 26, btn_cancel_trig.y + 10 ), 15 );

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

// ============================================================================
// MODAL DE SALVAR ESQUEMA COM NOME
// ============================================================================

void mobile_ui_system::draw_save_scheme_modal( const SDL_Renderer_Ptr &renderer, Font *font )
{
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    SDL_Rect overlay = { 0, 0, screen_width, screen_height };
    SetRenderDrawColor( renderer, 5, 8, 15, 225 );
    RenderFillRect( renderer, &overlay );

    int m_w = std::min( 860, static_cast<int>( screen_width * 0.94f ) );
    int m_h = std::min( 520, static_cast<int>( screen_height * 0.94f ) );
    SDL_Rect modal_box = { ( screen_width - m_w ) / 2, ( screen_height - m_h ) / 2, m_w, m_h };

    SetRenderDrawColor( renderer, 14, 20, 34, 255 );
    RenderFillRect( renderer, &modal_box );
    SetRenderDrawColor( renderer, 45, 130, 230, 255 );
    RenderDrawRect( renderer, &modal_box );

    font->OutputChar( renderer, _( "SALVAR ESQUEMA DE INTERFACE - DIGITE O NOME" ), point( modal_box.x + 24, modal_box.y + 16 ), 11 );

    // Campo de texto exibindo o nome digitado
    SDL_Rect text_box = { modal_box.x + 24, modal_box.y + 50, modal_box.w - 180, 46 };
    SetRenderDrawColor( renderer, 10, 16, 28, 255 );
    RenderFillRect( renderer, &text_box );
    SetRenderDrawColor( renderer, 60, 160, 255, 255 );
    RenderDrawRect( renderer, &text_box );

    std::string display_name = temp_scheme_name_input.empty() ? _( "Meu Esquema" ) : temp_scheme_name_input;
    font->OutputChar( renderer, display_name + "_", point( text_box.x + 14, text_box.y + 14 ), 11 );

    // Botao Backspace [ <- APAGAR ]
    SDL_Rect btn_backspace = { text_box.x + text_box.w + 10, text_box.y, 122, 46 };
    SetRenderDrawColor( renderer, 140, 45, 45, 240 );
    RenderFillRect( renderer, &btn_backspace );
    SetRenderDrawColor( renderer, 220, 80, 80, 255 );
    RenderDrawRect( renderer, &btn_backspace );
    font->OutputChar( renderer, _( "<- APAGAR" ), point( btn_backspace.x + 12, btn_backspace.y + 14 ), 15 );

    // Presets Rapidos de 1 Toque
    int pres_y = text_box.y + text_box.h + 10;
    font->OutputChar( renderer, _( "Sugestões Rápidas:" ), point( modal_box.x + 24, pres_y + 8 ), 14 );

    std::vector<std::string> presets = { "Combate", "Exploracao", "Veiculos", "Limpo", "Survival" };
    int px_cur = modal_box.x + 170;
    for( const auto &p : presets ) {
        int pw = 110;
        SDL_Rect prc = { px_cur, pres_y, pw, 32 };
        SetRenderDrawColor( renderer, 25, 40, 60, 230 );
        RenderFillRect( renderer, &prc );
        SetRenderDrawColor( renderer, 60, 100, 150, 255 );
        RenderDrawRect( renderer, &prc );
        font->OutputChar( renderer, p, point( prc.x + 10, prc.y + 8 ), 15 );
        px_cur += pw + 8;
    }

    // Teclado Virtual Tatil Completo
    int kb_y = pres_y + 44;
    std::vector<std::string> rows = {
        "QWERTYUIOP",
        "ASDFGHJKL",
        "ZXCVBNM-_"
    };

    int key_w = ( modal_box.w - 48 - 45 ) / 10;
    int key_h = 38;
    int k_gap = 5;

    for( size_t r = 0; r < rows.size(); ++r ) {
        const std::string &row_str = rows[r];
        int start_x = modal_box.x + 24 + ( ( modal_box.w - 48 ) - ( static_cast<int>( row_str.size() ) * ( key_w + k_gap ) ) ) / 2;
        int ry = kb_y + static_cast<int>( r ) * ( key_h + k_gap );

        for( size_t c = 0; c < row_str.size(); ++c ) {
            SDL_Rect k_rc = { start_x + static_cast<int>( c ) * ( key_w + k_gap ), ry, key_w, key_h };
            SetRenderDrawColor( renderer, 24, 34, 52, 240 );
            RenderFillRect( renderer, &k_rc );
            SetRenderDrawColor( renderer, 55, 80, 120, 255 );
            RenderDrawRect( renderer, &k_rc );

            std::string ch( 1, row_str[c] );
            int kx = k_rc.x + ( k_rc.w - font->width ) / 2;
            int ky = k_rc.y + ( k_rc.h - font->height ) / 2;
            font->OutputChar( renderer, ch, point( kx, ky ), 15 );
        }
    }

    // Linha de Numeros e Simbolos
    int num_y = kb_y + 3 * ( key_h + k_gap );
    std::string num_str = "1234567890";
    int start_nx = modal_box.x + 24 + ( ( modal_box.w - 48 ) - ( static_cast<int>( num_str.size() ) * ( key_w + k_gap ) ) ) / 2;
    for( size_t c = 0; c < num_str.size(); ++c ) {
        SDL_Rect k_rc = { start_nx + static_cast<int>( c ) * ( key_w + k_gap ), num_y, key_w, key_h };
        SetRenderDrawColor( renderer, 24, 34, 52, 240 );
        RenderFillRect( renderer, &k_rc );
        SetRenderDrawColor( renderer, 55, 80, 120, 255 );
        RenderDrawRect( renderer, &k_rc );

        std::string ch( 1, num_str[c] );
        int kx = k_rc.x + ( k_rc.w - font->width ) / 2;
        int ky = k_rc.y + ( k_rc.h - font->height ) / 2;
        font->OutputChar( renderer, ch, point( kx, ky ), 11 );
    }

    // Linha de Espaco e Limpar Tudo
    int space_y = num_y + key_h + k_gap;
    SDL_Rect btn_space = { modal_box.x + ( modal_box.w - 240 ) / 2 - 80, space_y, 240, key_h };
    SetRenderDrawColor( renderer, 32, 45, 68, 240 );
    RenderFillRect( renderer, &btn_space );
    SetRenderDrawColor( renderer, 70, 110, 160, 255 );
    RenderDrawRect( renderer, &btn_space );
    font->OutputChar( renderer, _( "[ ESPAÇO ]" ), point( btn_space.x + 70, btn_space.y + 10 ), 15 );

    SDL_Rect btn_clear_all = { btn_space.x + btn_space.w + 12, space_y, 140, key_h };
    SetRenderDrawColor( renderer, 70, 35, 35, 240 );
    RenderFillRect( renderer, &btn_clear_all );
    SetRenderDrawColor( renderer, 160, 70, 70, 255 );
    RenderDrawRect( renderer, &btn_clear_all );
    font->OutputChar( renderer, _( "LIMPAR TUDO" ), point( btn_clear_all.x + 18, btn_clear_all.y + 10 ), 15 );

    // Botoes Inferiores: [ SALVAR ESQUEMA ] e [ CANCELAR ]
    int act_y = modal_box.y + modal_box.h - 58;
    SDL_Rect btn_ok = { modal_box.x + 24, act_y, 240, 44 };
    SetRenderDrawColor( renderer, 35, 150, 85, 255 );
    RenderFillRect( renderer, &btn_ok );
    SetRenderDrawColor( renderer, 50, 220, 120, 255 );
    RenderDrawRect( renderer, &btn_ok );
    font->OutputChar( renderer, _( "[ SALVAR ESQUEMA ]" ), point( btn_ok.x + 30, btn_ok.y + 12 ), 15 );

    SDL_Rect btn_cancel = { btn_ok.x + btn_ok.w + 16, act_y, 160, 44 };
    SetRenderDrawColor( renderer, 60, 25, 25, 240 );
    RenderFillRect( renderer, &btn_cancel );
    SetRenderDrawColor( renderer, 150, 50, 50, 255 );
    RenderDrawRect( renderer, &btn_cancel );
    font->OutputChar( renderer, _( "CANCELAR" ), point( btn_cancel.x + 36, btn_cancel.y + 12 ), 15 );

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}
bool mobile_ui_system::handle_schemes_tab_touch( float x, float y )
{
    if( selected_scheme_idx < 0 || selected_scheme_idx >= static_cast<int>( loaded_schemes.size() ) ) {
        selected_scheme_idx = 0;
    }

    auto &cur_scheme = loaded_schemes[selected_scheme_idx];

    // 1. Botao [ + NOVO ESQUEMA ]
    if( x >= schemes_btn_new_scheme.x && x <= schemes_btn_new_scheme.x + schemes_btn_new_scheme.w &&
        y >= schemes_btn_new_scheme.y && y <= schemes_btn_new_scheme.y + schemes_btn_new_scheme.h ) {
        interface_scheme ns;
        ns.id = string_format( "esquema_%d", static_cast<int>( loaded_schemes.size() ) );
        ns.name = string_format( _( "Novo Esquema %d" ), static_cast<int>( loaded_schemes.size() ) );
        ns.description = _( "Esquema personalizado de gatilhos tatil." );
        ns.is_default = false;
        ns.show_analog = true;
        ns.show_fixed_buttons = true;
        ns.show_context_slots = true;
        loaded_schemes.push_back( ns );
        selected_scheme_idx = static_cast<int>( loaded_schemes.size() ) - 1;
        save_current_as_temporary();
        schemes_feedback_msg = string_format( _( "Esquema '%s' criado! Toque em [+ GATILHO] para configurar." ), ns.name );
        return true;
    }

    // 2. Toque nos cards da lista de esquemas
    for( size_t i = 0; i < schemes_cards_rects.size(); ++i ) {
        const auto &rc = schemes_cards_rects[i];
        if( x >= rc.x && x <= rc.x + rc.w && y >= rc.y && y <= rc.y + rc.h ) {
            selected_scheme_idx = scheme_list_scroll_offset + static_cast<int>( i );
            trigger_list_scroll_offset = 0;
            return true;
        }
    }

    // 3. Rolagem da lista de esquemas
    if( x >= schemes_btn_scroll_up.x && x <= schemes_btn_scroll_up.x + schemes_btn_scroll_up.w &&
        y >= schemes_btn_scroll_up.y && y <= schemes_btn_scroll_up.y + schemes_btn_scroll_up.h ) {
        if( scheme_list_scroll_offset > 0 ) scheme_list_scroll_offset--;
        return true;
    }
    if( x >= schemes_btn_scroll_down.x && x <= schemes_btn_scroll_down.x + schemes_btn_scroll_down.w &&
        y >= schemes_btn_scroll_down.y && y <= schemes_btn_scroll_down.y + schemes_btn_scroll_down.h ) {
        if( scheme_list_scroll_offset + 1 < static_cast<int>( loaded_schemes.size() ) ) scheme_list_scroll_offset++;
        return true;
    }

    // 4. Botoes de Acao da Coluna Direita
    // [ APLICAR NO JOGO ]
    if( x >= scheme_btn_apply.x && x <= scheme_btn_apply.x + scheme_btn_apply.w &&
        y >= scheme_btn_apply.y && y <= scheme_btn_apply.y + scheme_btn_apply.h ) {
        apply_scheme( selected_scheme_idx );
        return true;
    }

    // [ + GATILHO ]
    if( x >= scheme_btn_add_trigger.x && x <= scheme_btn_add_trigger.x + scheme_btn_add_trigger.w &&
        y >= scheme_btn_add_trigger.y && y <= scheme_btn_add_trigger.y + scheme_btn_add_trigger.h ) {
        if( cur_scheme.is_default ) {
            schemes_feedback_msg = _( "O Esquema Padrão é de fábrica e protegido contra edição. Toque em [+ NOVO ESQUEMA] para personalizar!" );
            return true;
        }
        temp_editing_trigger = custom_trigger();
        temp_editing_trigger.id = string_format( "trig_%d", static_cast<int>( loaded_schemes[selected_scheme_idx].triggers.size() ) );
        temp_editing_trigger.label = "AÇÃO";
        trigger_config_modal_open = true;
        return true;
    }

    // [ POSICIONAR NA TELA ]
    if( x >= scheme_btn_position_screen.x && x <= scheme_btn_position_screen.x + scheme_btn_position_screen.w &&
        y >= scheme_btn_position_screen.y && y <= scheme_btn_position_screen.y + scheme_btn_position_screen.h ) {
        if( cur_scheme.is_default ) {
            schemes_feedback_msg = _( "O Esquema Padrão é de fábrica e não pode ser alterado." );
            return true;
        }
        if( cur_scheme.triggers.empty() ) {
            custom_trigger def_trig;
            def_trig.label = "TIRO";
            def_trig.keys = { 'f', 'f' };
            cur_scheme.triggers.push_back( def_trig );
        }
        settings_open = false;
        is_positioning_triggers = true;
        selected_trigger_idx = 0;
        return true;
    }

    // [ SALVAR ESQUEMA ]
    if( x >= scheme_btn_save_as.x && x <= scheme_btn_save_as.x + scheme_btn_save_as.w &&
        y >= scheme_btn_save_as.y && y <= scheme_btn_save_as.y + scheme_btn_save_as.h ) {
        temp_scheme_name_input = cur_scheme.name;
        save_scheme_modal_open = true;
        return true;
    }

    // [ LIMPAR ESQUEMA ]
    if( x >= scheme_btn_clear_scheme.x && x <= scheme_btn_clear_scheme.x + scheme_btn_clear_scheme.w &&
        y >= scheme_btn_clear_scheme.y && y <= scheme_btn_clear_scheme.y + scheme_btn_clear_scheme.h ) {
        if( cur_scheme.is_default ) {
            schemes_feedback_msg = _( "O Esquema Padrão é protegido de fábrica e não pode ser limpo ou alterado." );
            return true;
        }
        cur_scheme.triggers.clear();
        cur_scheme.show_analog = false;
        cur_scheme.show_fixed_buttons = false;
        cur_scheme.show_context_slots = false;
        save_current_as_temporary();
        if( !cur_scheme.id.empty() && cur_scheme.id != "padrao" ) {
            save_scheme_to_disk( cur_scheme );
        }
        schemes_feedback_msg = _( "Esquema limpo com sucesso! Apenas o Hub de Menus foi mantido em tela." );
        return true;
    }

    // [ EXCLUIR ESQUEMA ]
    if( x >= scheme_btn_delete.x && x <= scheme_btn_delete.x + scheme_btn_delete.w &&
        y >= scheme_btn_delete.y && y <= scheme_btn_delete.y + scheme_btn_delete.h ) {
        if( cur_scheme.is_default ) {
            schemes_feedback_msg = _( "O Esquema Padrão é protegido de fábrica e não pode ser excluído." );
            return true;
        }
        delete_scheme_from_disk( selected_scheme_idx );
        return true;
    }

    // [ TOGGLE ANALOGICO ]
    if( x >= scheme_btn_toggle_analog.x && x <= scheme_btn_toggle_analog.x + scheme_btn_toggle_analog.w &&
        y >= scheme_btn_toggle_analog.y && y <= scheme_btn_toggle_analog.y + scheme_btn_toggle_analog.h ) {
        if( cur_scheme.is_default ) {
            schemes_feedback_msg = _( "O Esquema Padrão sempre mantém o Analógico ativo." );
            return true;
        }
        cur_scheme.show_analog = !cur_scheme.show_analog;
        save_current_as_temporary();
        if( !cur_scheme.id.empty() && cur_scheme.id != "padrao" ) { save_scheme_to_disk( cur_scheme ); }
        schemes_feedback_msg = cur_scheme.show_analog ? _( "Analógico ativado neste esquema." ) : _( "Analógico removido deste esquema." );
        return true;
    }

    // [ TOGGLE BOTOES BASE ]
    if( x >= scheme_btn_toggle_fixed.x && x <= scheme_btn_toggle_fixed.x + scheme_btn_toggle_fixed.w &&
        y >= scheme_btn_toggle_fixed.y && y <= scheme_btn_toggle_fixed.y + scheme_btn_toggle_fixed.h ) {
        if( cur_scheme.is_default ) {
            schemes_feedback_msg = _( "O Esquema Padrão sempre mantém os Botões de Base ativos." );
            return true;
        }
        cur_scheme.show_fixed_buttons = !cur_scheme.show_fixed_buttons;
        save_current_as_temporary();
        if( !cur_scheme.id.empty() && cur_scheme.id != "padrao" ) { save_scheme_to_disk( cur_scheme ); }
        schemes_feedback_msg = cur_scheme.show_fixed_buttons ? _( "Botões de base ativados neste esquema." ) : _( "Botões de base removidos (apenas Hub mantido em tela)." );
        return true;
    }

    // 5. Excluir gatilho individual na lista
    for( size_t i = 0; i < trigger_item_delete_rects.size(); ++i ) {
        const auto &d_rc = trigger_item_delete_rects[i];
        if( x >= d_rc.x && x <= d_rc.x + d_rc.w && y >= d_rc.y && y <= d_rc.y + d_rc.h ) {
            if( cur_scheme.is_default ) {
                schemes_feedback_msg = _( "O Esquema Padrão não pode ter gatilhos excluídos." );
                return true;
            }
            size_t trig_idx = trigger_list_scroll_offset + i;
            if( trig_idx < cur_scheme.triggers.size() ) {
                cur_scheme.triggers.erase( cur_scheme.triggers.begin() + trig_idx );
                save_current_as_temporary();
                schemes_feedback_msg = _( "Gatilho removido." );
                return true;
            }
        }
    }

    return true;
}
bool mobile_ui_system::handle_trigger_config_touch( float x, float y )
{
    int m_w = std::min( 920, static_cast<int>( screen_width * 0.94f ) );
    int m_h = std::min( 620, static_cast<int>( screen_height * 0.94f ) );
    SDL_Rect modal_box = { ( screen_width - m_w ) / 2, ( screen_height - m_h ) / 2, m_w, m_h };

    int ty = modal_box.y + 14;
    int lbl_y = ty + 56;
    std::vector<std::string> quick_labels = { "TIRO", "INV", "RECARGA", "OLHAR", "MAPA", "ESPERAR", "ENTER", "ESC", "C1", "C2" };
    int qx = modal_box.x + 130;
    for( const auto &ql : quick_labels ) {
        int w = 50;
        if( x >= qx && x <= qx + w && y >= lbl_y && y <= lbl_y + 26 ) {
            temp_editing_trigger.label = ql;
            return true;
        }
        qx += w + 6;
    }

    // Abas de categorias
    int cat_y = lbl_y + 34;
    int cat_w = ( modal_box.w - 44 - 30 ) / 6;
    for( int i = 0; i < 6; ++i ) {
        SDL_Rect cat_btn = { modal_box.x + 22 + i * ( cat_w + 6 ), cat_y, cat_w, 32 };
        if( x >= cat_btn.x && x <= cat_btn.x + cat_btn.w && y >= cat_btn.y && y <= cat_btn.y + cat_btn.h ) {
            trigger_keyboard_category = i;
            return true;
        }
    }

    // Teclas no grid
    int grid_y = cat_y + 40;
    std::vector<int> current_keys_codes;
    if( trigger_keyboard_category == 0 ) {
        for( char c = 'a'; c <= 'z'; ++c ) current_keys_codes.push_back( static_cast<int>( c ) );
    } else if( trigger_keyboard_category == 1 ) {
        for( char c = 'A'; c <= 'Z'; ++c ) current_keys_codes.push_back( static_cast<int>( c ) );
    } else if( trigger_keyboard_category == 2 ) {
        for( char c = '0'; c <= '9'; ++c ) current_keys_codes.push_back( static_cast<int>( c ) );
    } else if( trigger_keyboard_category == 3 ) {
        std::string syms = "!@#$%^&*()_+-=[]{}|;':\",./<>?~`";
        for( char c : syms ) current_keys_codes.push_back( static_cast<int>( c ) );
    } else if( trigger_keyboard_category == 4 ) {
        current_keys_codes = { '\n', 27, ' ', '\t', 127, KEY_DC, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_PPAGE, KEY_NPAGE, KEY_HOME, KEY_END };
    } else if( trigger_keyboard_category == 5 ) {
        for( int f = 1; f <= 12; ++f ) current_keys_codes.push_back( KEY_F( f ) );
    }

    int k_btn_w = 54;
    int k_btn_h = 36;
    int k_gap = 6;
    int cols_per_row = ( modal_box.w - 44 ) / ( k_btn_w + k_gap );

    for( size_t i = 0; i < current_keys_codes.size(); ++i ) {
        int r = static_cast<int>( i ) / cols_per_row;
        int c = static_cast<int>( i ) % cols_per_row;
        int kx = modal_box.x + 22 + c * ( k_btn_w + k_gap );
        int ky = grid_y + r * ( k_btn_h + k_gap );

        if( x >= kx && x <= kx + k_btn_w && y >= ky && y <= ky + k_btn_h ) {
            temp_editing_trigger.keys.push_back( current_keys_codes[i] );
            if( temp_editing_trigger.label == "AÇÃO" && current_keys_codes[i] >= 32 && current_keys_codes[i] <= 126 ) {
                temp_editing_trigger.label = std::string( 1, static_cast<char>( current_keys_codes[i] ) );
            }
            return true;
        }
    }

    // Controles de Tamanho, Opacidade e Cor
    int foot_y = modal_box.y + modal_box.h - 90;

    // Tamanho [-] e [+]
    if( y >= foot_y && y <= foot_y + 30 ) {
        if( x >= modal_box.x + 120 && x <= modal_box.x + 160 ) {
            temp_editing_trigger.size = std::clamp( temp_editing_trigger.size - 6, 36, 120 );
            return true;
        }
        if( x >= modal_box.x + 170 && x <= modal_box.x + 210 ) {
            temp_editing_trigger.size = std::clamp( temp_editing_trigger.size + 6, 36, 120 );
            return true;
        }
        // Opacidade [-] e [+]
        if( x >= modal_box.x + 400 && x <= modal_box.x + 440 ) {
            temp_editing_trigger.opacity = std::clamp( temp_editing_trigger.opacity - 0.1f, 0.2f, 1.0f );
            return true;
        }
        if( x >= modal_box.x + 450 && x <= modal_box.x + 490 ) {
            temp_editing_trigger.opacity = std::clamp( temp_editing_trigger.opacity + 0.1f, 0.2f, 1.0f );
            return true;
        }
        // Alternar Cor
        if( x >= modal_box.x + 650 && x <= modal_box.x + 780 ) {
            temp_editing_trigger.color_theme = ( temp_editing_trigger.color_theme + 1 ) % 6;
            return true;
        }
    }

    // Botoes finais: [ SALVAR GATILHO ], [ LIMPAR TECLAS ], [ CANCELAR ]
    int act_y = foot_y + 40;
    SDL_Rect btn_save_trig = { modal_box.x + 22, act_y, 240, 38 };
    if( x >= btn_save_trig.x && x <= btn_save_trig.x + btn_save_trig.w &&
        y >= btn_save_trig.y && y <= btn_save_trig.y + btn_save_trig.h ) {
        if( selected_scheme_idx >= 0 && selected_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
            loaded_schemes[selected_scheme_idx].triggers.push_back( temp_editing_trigger );
            save_current_as_temporary();
            schemes_feedback_msg = string_format( _( "Gatilho '%s' adicionado com sucesso!" ), temp_editing_trigger.label );
        }
        trigger_config_modal_open = false;
        return true;
    }

    SDL_Rect btn_clear_keys = { btn_save_trig.x + btn_save_trig.w + 14, act_y, 180, 38 };
    if( x >= btn_clear_keys.x && x <= btn_clear_keys.x + btn_clear_keys.w &&
        y >= btn_clear_keys.y && y <= btn_clear_keys.y + btn_clear_keys.h ) {
        temp_editing_trigger.keys.clear();
        return true;
    }

    SDL_Rect btn_cancel_trig = { btn_clear_keys.x + btn_clear_keys.w + 14, act_y, 140, 38 };
    if( x >= btn_cancel_trig.x && x <= btn_cancel_trig.x + btn_cancel_trig.w &&
        y >= btn_cancel_trig.y && y <= btn_cancel_trig.y + btn_cancel_trig.h ) {
        trigger_config_modal_open = false;
        return true;
    }

    return true;
}

bool mobile_ui_system::handle_save_scheme_touch( float x, float y )
{
    int m_w = std::min( 860, static_cast<int>( screen_width * 0.94f ) );
    int m_h = std::min( 520, static_cast<int>( screen_height * 0.94f ) );
    SDL_Rect modal_box = { ( screen_width - m_w ) / 2, ( screen_height - m_h ) / 2, m_w, m_h };

    SDL_Rect text_box = { modal_box.x + 24, modal_box.y + 50, modal_box.w - 180, 46 };
    SDL_Rect btn_backspace = { text_box.x + text_box.w + 10, text_box.y, 122, 46 };

    // Toque no Backspace
    if( x >= btn_backspace.x && x <= btn_backspace.x + btn_backspace.w &&
        y >= btn_backspace.y && y <= btn_backspace.y + btn_backspace.h ) {
        if( !temp_scheme_name_input.empty() ) {
            temp_scheme_name_input.pop_back();
        }
        return true;
    }

    // Presets Rapidos
    int pres_y = text_box.y + text_box.h + 10;
    std::vector<std::string> presets = { "Combate", "Exploracao", "Veiculos", "Limpo", "Survival" };
    int px_cur = modal_box.x + 170;
    for( const auto &p : presets ) {
        int pw = 110;
        SDL_Rect prc = { px_cur, pres_y, pw, 32 };
        if( x >= prc.x && x <= prc.x + prc.w && y >= prc.y && y <= prc.y + prc.h ) {
            temp_scheme_name_input = p;
            return true;
        }
        px_cur += pw + 8;
    }

    // Teclado Virtual Tatil
    int kb_y = pres_y + 44;
    std::vector<std::string> rows = {
        "QWERTYUIOP",
        "ASDFGHJKL",
        "ZXCVBNM-_"
    };

    int key_w = ( modal_box.w - 48 - 45 ) / 10;
    int key_h = 38;
    int k_gap = 5;

    for( size_t r = 0; r < rows.size(); ++r ) {
        const std::string &row_str = rows[r];
        int start_x = modal_box.x + 24 + ( ( modal_box.w - 48 ) - ( static_cast<int>( row_str.size() ) * ( key_w + k_gap ) ) ) / 2;
        int ry = kb_y + static_cast<int>( r ) * ( key_h + k_gap );

        for( size_t c = 0; c < row_str.size(); ++c ) {
            SDL_Rect k_rc = { start_x + static_cast<int>( c ) * ( key_w + k_gap ), ry, key_w, key_h };
            if( x >= k_rc.x && x <= k_rc.x + k_rc.w && y >= k_rc.y && y <= k_rc.y + k_rc.h ) {
                if( temp_scheme_name_input.size() < 28 ) {
                    temp_scheme_name_input += row_str[c];
                }
                return true;
            }
        }
    }

    // Numeros
    int num_y = kb_y + 3 * ( key_h + k_gap );
    std::string num_str = "1234567890";
    int start_nx = modal_box.x + 24 + ( ( modal_box.w - 48 ) - ( static_cast<int>( num_str.size() ) * ( key_w + k_gap ) ) ) / 2;
    for( size_t c = 0; c < num_str.size(); ++c ) {
        SDL_Rect k_rc = { start_nx + static_cast<int>( c ) * ( key_w + k_gap ), num_y, key_w, key_h };
        if( x >= k_rc.x && x <= k_rc.x + k_rc.w && y >= k_rc.y && y <= k_rc.y + k_rc.h ) {
            if( temp_scheme_name_input.size() < 28 ) {
                temp_scheme_name_input += num_str[c];
            }
            return true;
        }
    }

    // Espaco e Limpar Tudo
    int space_y = num_y + key_h + k_gap;
    SDL_Rect btn_space = { modal_box.x + ( modal_box.w - 240 ) / 2 - 80, space_y, 240, key_h };
    if( x >= btn_space.x && x <= btn_space.x + btn_space.w && y >= btn_space.y && y <= btn_space.y + btn_space.h ) {
        if( temp_scheme_name_input.size() < 28 && !temp_scheme_name_input.empty() ) {
            temp_scheme_name_input += " ";
        }
        return true;
    }

    SDL_Rect btn_clear_all = { btn_space.x + btn_space.w + 12, space_y, 140, key_h };
    if( x >= btn_clear_all.x && x <= btn_clear_all.x + btn_clear_all.w && y >= btn_clear_all.y && y <= btn_clear_all.y + btn_clear_all.h ) {
        temp_scheme_name_input.clear();
        return true;
    }

    // Botoes Inferiores: [ SALVAR ESQUEMA ] e [ CANCELAR ]
    int act_y = modal_box.y + modal_box.h - 58;
    SDL_Rect btn_ok = { modal_box.x + 24, act_y, 240, 44 };
    if( x >= btn_ok.x && x <= btn_ok.x + btn_ok.w && y >= btn_ok.y && y <= btn_ok.y + btn_ok.h ) {
        if( selected_scheme_idx >= 0 && selected_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
            auto new_scheme = loaded_schemes[selected_scheme_idx];
            new_scheme.id = "esquema_" + std::to_string( std::time( nullptr ) % 100000 );
            new_scheme.name = temp_scheme_name_input.empty() ? _( "Meu Esquema" ) : temp_scheme_name_input;
            new_scheme.is_default = false;
            save_scheme_to_disk( new_scheme );
            load_all_schemes();
            selected_scheme_idx = static_cast<int>( loaded_schemes.size() ) - 1;
            apply_scheme( selected_scheme_idx );
            schemes_feedback_msg = string_format( _( "Esquema '%s' salvo em Documentos/CataclysmDDA/schemes!" ), new_scheme.name );
        }
        save_scheme_modal_open = false;
        return true;
    }

    SDL_Rect btn_cancel = { btn_ok.x + btn_ok.w + 16, act_y, 160, 44 };
    if( x >= btn_cancel.x && x <= btn_cancel.x + btn_cancel.w && y >= btn_cancel.y && y <= btn_cancel.y + btn_cancel.h ) {
        save_scheme_modal_open = false;
        return true;
    }

    return true;
}


// ============================================================================
// MODO DE POSICIONAMENTO TATIL NA TELA (DRAG & DROP)
// ============================================================================

void mobile_ui_system::draw_positioning_mode( const SDL_Renderer_Ptr &renderer, Font *font )
{
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    if( selected_scheme_idx < 0 || selected_scheme_idx >= static_cast<int>( loaded_schemes.size() ) ) {
        is_positioning_triggers = false;
        return;
    }

    auto &cur_scheme = loaded_schemes[selected_scheme_idx];

    // 1. Desenha todos os gatilhos deste esquema na tela
    for( size_t i = 0; i < cur_scheme.triggers.size(); ++i ) {
        auto &trig = cur_scheme.triggers[i];
        trig.calculated_bounds.w = static_cast<int>( trig.size * density_scale );
        trig.calculated_bounds.h = static_cast<int>( trig.size * density_scale );
        trig.calculated_bounds.x = static_cast<int>( trig.x_pct * screen_width ) - trig.calculated_bounds.w / 2;
        trig.calculated_bounds.y = static_cast<int>( trig.y_pct * screen_height ) - trig.calculated_bounds.h / 2;

        bool is_selected = ( static_cast<int>( i ) == selected_trigger_idx );
        draw_custom_trigger_button( renderer, font, trig, is_selected );
    }

    // 2. Barra Flutuante de Ferramentas no Topo da Tela
    int bar_w = std::min( 860, static_cast<int>( screen_width * 0.96f ) );
    int bar_h = 56;
    pos_bar_rect = { ( screen_width - bar_w ) / 2, 14, bar_w, bar_h };

    SetRenderDrawColor( renderer, 10, 16, 28, 240 );
    RenderFillRect( renderer, &pos_bar_rect );
    SetRenderDrawColor( renderer, 255, 215, 0, 255 ); // Dourado brilhante
    RenderDrawRect( renderer, &pos_bar_rect );

    // Texto de instrucao a esquerda
    font->OutputChar( renderer, _( "MODO ARRASTE: Toque e mova qualquer gatilho na tela" ), point( pos_bar_rect.x + 14, pos_bar_rect.y + 12 ), 11 );

    // Botoes de ajuste fino do gatilho selecionado
    int bx = pos_bar_rect.x + 14;
    int by = pos_bar_rect.y + 28;

    pos_btn_size_minus = { bx, by, 60, 22 };
    pos_btn_size_plus = { bx + 64, by, 60, 22 };
    pos_btn_opacity_minus = { bx + 136, by, 60, 22 };
    pos_btn_opacity_plus = { bx + 200, by, 60, 22 };
    pos_btn_color = { bx + 272, by, 90, 22 };
    pos_btn_done = { pos_bar_rect.x + pos_bar_rect.w - 150, pos_bar_rect.y + 10, 136, 36 };

    // Fundo dos mini-botoes
    SetRenderDrawColor( renderer, 25, 38, 58, 255 );
    RenderFillRect( renderer, &pos_btn_size_minus );
    RenderFillRect( renderer, &pos_btn_size_plus );
    RenderFillRect( renderer, &pos_btn_opacity_minus );
    RenderFillRect( renderer, &pos_btn_opacity_plus );
    RenderFillRect( renderer, &pos_btn_color );

    SetRenderDrawColor( renderer, 60, 95, 140, 255 );
    RenderDrawRect( renderer, &pos_btn_size_minus );
    RenderDrawRect( renderer, &pos_btn_size_plus );
    RenderDrawRect( renderer, &pos_btn_opacity_minus );
    RenderDrawRect( renderer, &pos_btn_opacity_plus );
    RenderDrawRect( renderer, &pos_btn_color );

    font->OutputChar( renderer, "TAM -", point( pos_btn_size_minus.x + 6, pos_btn_size_minus.y + 4 ), 15 );
    font->OutputChar( renderer, "TAM +", point( pos_btn_size_plus.x + 6, pos_btn_size_plus.y + 4 ), 15 );
    font->OutputChar( renderer, "OPA -", point( pos_btn_opacity_minus.x + 6, pos_btn_opacity_minus.y + 4 ), 15 );
    font->OutputChar( renderer, "OPA +", point( pos_btn_opacity_plus.x + 6, pos_btn_opacity_plus.y + 4 ), 15 );
    font->OutputChar( renderer, "COR", point( pos_btn_color.x + 28, pos_btn_color.y + 4 ), 11 );

    // Botao [ CONCLUIR ] em verde vibrante
    SetRenderDrawColor( renderer, 35, 150, 85, 255 );
    RenderFillRect( renderer, &pos_btn_done );
    SetRenderDrawColor( renderer, 60, 230, 130, 255 );
    RenderDrawRect( renderer, &pos_btn_done );
    font->OutputChar( renderer, _( "[ CONCLUIR ]" ), point( pos_btn_done.x + 14, pos_btn_done.y + 9 ), 15 );

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

bool mobile_ui_system::handle_positioning_touch_down( float x, float y, int finger_id )
{
    // 1. Checa toques na barra flutuante de ferramentas no topo
    if( x >= pos_btn_done.x && x <= pos_btn_done.x + pos_btn_done.w &&
        y >= pos_btn_done.y && y <= pos_btn_done.y + pos_btn_done.h ) {
        save_current_as_temporary();
        is_positioning_triggers = false;
        settings_open = true;
        return true;
    }

    if( selected_scheme_idx >= 0 && selected_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        auto &cur_scheme = loaded_schemes[selected_scheme_idx];
        if( selected_trigger_idx >= 0 && selected_trigger_idx < static_cast<int>( cur_scheme.triggers.size() ) ) {
            auto &trig = cur_scheme.triggers[selected_trigger_idx];
            // Tamanho -
            if( x >= pos_btn_size_minus.x && x <= pos_btn_size_minus.x + pos_btn_size_minus.w &&
                y >= pos_btn_size_minus.y && y <= pos_btn_size_minus.y + pos_btn_size_minus.h ) {
                trig.size = std::clamp( trig.size - 6, 36, 120 );
                save_current_as_temporary();
                return true;
            }
            // Tamanho +
            if( x >= pos_btn_size_plus.x && x <= pos_btn_size_plus.x + pos_btn_size_plus.w &&
                y >= pos_btn_size_plus.y && y <= pos_btn_size_plus.y + pos_btn_size_plus.h ) {
                trig.size = std::clamp( trig.size + 6, 36, 120 );
                save_current_as_temporary();
                return true;
            }
            // Opacidade -
            if( x >= pos_btn_opacity_minus.x && x <= pos_btn_opacity_minus.x + pos_btn_opacity_minus.w &&
                y >= pos_btn_opacity_minus.y && y <= pos_btn_opacity_minus.y + pos_btn_opacity_minus.h ) {
                trig.opacity = std::clamp( trig.opacity - 0.1f, 0.2f, 1.0f );
                save_current_as_temporary();
                return true;
            }
            // Opacidade +
            if( x >= pos_btn_opacity_plus.x && x <= pos_btn_opacity_plus.x + pos_btn_opacity_plus.w &&
                y >= pos_btn_opacity_plus.y && y <= pos_btn_opacity_plus.y + pos_btn_opacity_plus.h ) {
                trig.opacity = std::clamp( trig.opacity + 0.1f, 0.2f, 1.0f );
                save_current_as_temporary();
                return true;
            }
            // Alternar Cor
            if( x >= pos_btn_color.x && x <= pos_btn_color.x + pos_btn_color.w &&
                y >= pos_btn_color.y && y <= pos_btn_color.y + pos_btn_color.h ) {
                trig.color_theme = ( trig.color_theme + 1 ) % 6;
                save_current_as_temporary();
                return true;
            }
        }

        // 2. Checa se tocou em algum gatilho na tela para iniciar o arraste (drag & drop)
        for( size_t i = 0; i < cur_scheme.triggers.size(); ++i ) {
            auto &trig = cur_scheme.triggers[i];
            if( x >= trig.calculated_bounds.x && x <= trig.calculated_bounds.x + trig.calculated_bounds.w &&
                y >= trig.calculated_bounds.y && y <= trig.calculated_bounds.y + trig.calculated_bounds.h ) {
                selected_trigger_idx = static_cast<int>( i );
                dragging_trigger_idx = static_cast<int>( i );
                trig.active_finger_id = finger_id;
                return true;
            }
        }
    }

    return true;
}

bool mobile_ui_system::handle_positioning_touch_motion( float x, float y, int finger_id )
{
    if( selected_scheme_idx >= 0 && selected_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        auto &cur_scheme = loaded_schemes[selected_scheme_idx];
        if( dragging_trigger_idx >= 0 && dragging_trigger_idx < static_cast<int>( cur_scheme.triggers.size() ) ) {
            auto &trig = cur_scheme.triggers[dragging_trigger_idx];
            if( trig.active_finger_id == finger_id ) {
                // Atualiza as posicoes percentuais proporcionais em tempo real
                trig.x_pct = std::clamp( x / static_cast<float>( screen_width ), 0.05f, 0.95f );
                trig.y_pct = std::clamp( y / static_cast<float>( screen_height ), 0.08f, 0.95f );
                return true;
            }
        }
    }
    return true;
}

bool mobile_ui_system::handle_positioning_touch_up( float /*x*/, float /*y*/, int finger_id )
{
    if( selected_scheme_idx >= 0 && selected_scheme_idx < static_cast<int>( loaded_schemes.size() ) ) {
        auto &cur_scheme = loaded_schemes[selected_scheme_idx];
        if( dragging_trigger_idx >= 0 && dragging_trigger_idx < static_cast<int>( cur_scheme.triggers.size() ) ) {
            if( cur_scheme.triggers[dragging_trigger_idx].active_finger_id == finger_id ) {
                cur_scheme.triggers[dragging_trigger_idx].active_finger_id = -1;
                dragging_trigger_idx = -1;
                // Salva automaticamente imediatamente a mudanca!
                save_current_as_temporary();
                return true;
            }
        }
    }
    dragging_trigger_idx = -1;
    return true;
}

} // namespace cata_mobile
