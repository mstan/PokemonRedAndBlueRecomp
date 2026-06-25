/*
 * pokemon_overlay.cpp — Pokémon Red/Blue (and Gen 2) game-specific overlay.
 *
 * This is the former built-in "Pokemon Options" Esc-menu block, moved OUT of
 * the agnostic libgbrt core (where it was a hack) into this per-game fork.
 * It plugs into the runtime via two game_extras hooks:
 *   - game_draw_overlay(ctx): emits the ImGui "Pokemon Options" section inside
 *     the runtime's settings window (the core calls it once per frame, after
 *     its built-in sections — see runtime/src/platform_sdl.cpp).
 *   - game_on_frame(ctx): drives gb_wild_5050_tick() (was in poll_events).
 *
 * All cheat/event logic lives in the relocated mock_*.c modules (this fork's
 * pokemon/ dir); this TU is just the UI + per-frame wiring. ImGui is safe to
 * call here: the runtime and this TU share the one ImGui context (same libgbrt
 * link). Build wiring is in ../game_build.cmake.
 */
#include "imgui.h"

#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>

extern "C" {
#include "game_extras.h"
#include "gbrt.h"
#include "pokemon/mock_gen1.h"
#include "pokemon/mock_gen2.h"
#include "pokemon/mock_ir.h"
#include "pokemon/mock_inject_file.h"
#include "pokemon/mock_evolve_patch.h"
#include "pokemon/mock_wild_5050.h"
}

/* Directory under injects/ scanned for distribution files (.pkm/.pk1/.pk2).
 * Was g_active_game_id (asset-loader game_id) in the old core path; this fork
 * uses a fixed id since it ships a single Red/Blue build. */
static const char* const POKEMON_GAME_ID = "pokemon_red_blue";

/* Ephemeral menu input state (was file-scope in platform_sdl.cpp). */
static int          g_mg_item_idx = 0;
static int          g_mg_deco_idx = 0;
static const char*  g_odd_egg_last_msg = NULL;
static int          g_builder_species = 25;   /* Pikachu */
static int          g_builder_level = 50;
static bool         g_builder_shiny = false;
static const char*  g_builder_last_msg = NULL;
static int          g_event_sel_idx = 0;
static char         g_event_last_msg[160] = "";
static GBInjectFileEntry g_event_files[GB_INJECT_FILE_MAX];
static int          g_event_files_count = -1;  /* -1 sentinel = needs rescan */

/* ── Per-frame hook: wild-encounter 50/50 re-rolls on map change ─────────── */
extern "C" void game_on_frame(struct GBContext* ctx) {
    /* No-op when the toggle is disabled. */
    gb_wild_5050_tick(ctx);
}

/* ── ImGui overlay hook: the "Pokemon Options" section ──────────────────── */
extern "C" void game_draw_overlay(struct GBContext* ctx) {
    if (!ctx) return;

    /* Mobile Adapter event triggers — Crystal-only. We hit the relevant
     * SRAM/WRAM bytes directly (same end result as a HoF-time SRAM write). */
    bool crystal_active = gb_mock_crystal_active(ctx);

    /* Generic Pokémon builder — works on any Gen 1 (Red/Blue/Yellow) or
     * Gen 2 (Gold/Silver/Crystal) cart. Per-cart WRAM offsets and ROM
     * data-table addresses come from the mock_gen1/gen2 dispatch tables. */
    bool builder_gen2 = gb_mock_gen2_detect(ctx) != GB_MOCK_GEN2_NONE;
    bool builder_gen1 = !builder_gen2 &&
                        gb_mock_gen1_detect(ctx) != GB_MOCK_GEN1_NONE;
    if (!(builder_gen1 || builder_gen2)) return;

    int builder_species_count = builder_gen2
        ? GB_MOCK_GEN2_SPECIES_COUNT : GB_MOCK_GEN1_SPECIES_COUNT;
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Pokemon Options")) {
        ImGui::PushTextWrapPos(0.0f);

        /* Mystery Gift IR mock — any Gen 2 cart. Two carousels (items +
         * decorations) with Send buttons; the queued gift is delivered the
         * next time the cart enters Mystery Gift IR. */
        if (builder_gen2) {
            ImGui::TextDisabled("Mystery Gift");
            const float mg_button_width = ImGui::CalcTextSize("Send").x +
                ImGui::GetStyle().FramePadding.x * 2.0f;
            if (ImGui::ArrowButton("##mg_item_prev", ImGuiDir_Left)) {
                g_mg_item_idx = (g_mg_item_idx - 1 + GB_MOCK_IR_NUM_ITEMS) % GB_MOCK_IR_NUM_ITEMS;
            }
            ImGui::SameLine();
            ImGui::Text("Item: %-16s (%d/%d)",
                        gb_mock_ir_item_name(g_mg_item_idx),
                        g_mg_item_idx + 1, GB_MOCK_IR_NUM_ITEMS);
            ImGui::SameLine();
            if (ImGui::ArrowButton("##mg_item_next", ImGuiDir_Right)) {
                g_mg_item_idx = (g_mg_item_idx + 1) % GB_MOCK_IR_NUM_ITEMS;
            }
            ImGui::SameLine();
            if (ImGui::Button("Send##item", ImVec2(mg_button_width * 1.6f, 0.0f))) {
                gb_mock_ir_queue(GB_MOCK_IR_KIND_ITEM, g_mg_item_idx);
            }
            if (ImGui::ArrowButton("##mg_deco_prev", ImGuiDir_Left)) {
                g_mg_deco_idx = (g_mg_deco_idx - 1 + GB_MOCK_IR_NUM_DECOS) % GB_MOCK_IR_NUM_DECOS;
            }
            ImGui::SameLine();
            ImGui::Text("Deco: %-16s (%d/%d)",
                        gb_mock_ir_deco_name(g_mg_deco_idx),
                        g_mg_deco_idx + 1, GB_MOCK_IR_NUM_DECOS);
            ImGui::SameLine();
            if (ImGui::ArrowButton("##mg_deco_next", ImGuiDir_Right)) {
                g_mg_deco_idx = (g_mg_deco_idx + 1) % GB_MOCK_IR_NUM_DECOS;
            }
            ImGui::SameLine();
            if (ImGui::Button("Send##deco", ImVec2(mg_button_width * 1.6f, 0.0f))) {
                gb_mock_ir_queue(GB_MOCK_IR_KIND_DECO, g_mg_deco_idx);
            }
            ImGui::TextDisabled("Queued: %s", gb_mock_ir_queue_label());
            ImGui::Spacing();
        }

        /* Mobile Events (Crystal only) — GS Ball trigger and Odd Egg roller. */
        if (crystal_active) {
            ImGui::TextDisabled("Mobile Events");
            bool celebi_caught = gb_mock_crystal_celebi_caught(ctx);
            const char* button_label = celebi_caught
                ? "Re-arm GS Ball event##gsball"
                : "Trigger GS Ball event##gsball";
            if (ImGui::Button(button_label)) {
                gb_mock_crystal_apply_gs_ball(ctx);
            }
            uint8_t gs_flag = gb_mock_crystal_gs_ball_flag(ctx);
            ImGui::TextDisabled("Status: %s (sGSBallFlag = 0x%02X)",
                                gb_mock_crystal_gs_ball_state_label(ctx),
                                gs_flag);
            if (celebi_caught) {
                ImGui::TextDisabled("Celebi was already caught - pressing this re-arms the event so you can encounter it again.");
            }

            uint8_t party_count = gb_mock_gen2_party_count(ctx);
            if (ImGui::Button("Receive Odd Egg##odd_egg")) {
                if (gb_mock_crystal_apply_odd_egg(ctx)) {
                    g_odd_egg_last_msg = "Sent - check your party.";
                } else {
                    g_odd_egg_last_msg = "Party full (6/6) - release or store a Pokemon first.";
                }
            }
            if (g_odd_egg_last_msg) {
                ImGui::TextDisabled("%s", g_odd_egg_last_msg);
            } else {
                ImGui::TextDisabled("Party: %d/6. Rolls one of 7 baby Pokemon (Pichu/Cleffa/Igglybuff/Smoochum/Magby/Tyrogue/Elekid) with the original 14%% shiny rate.",
                                    party_count);
            }
            ImGui::Spacing();
        }

        ImGui::TextDisabled("Pokemon Builder");

        /* Species name cache — rebuilt when the active generation differs
         * from the cached one, so the UI can move between Gen 1 and Gen 2
         * carts (after Restart) without stale labels. */
        static std::vector<std::string> species_labels;
        static bool species_labels_gen2 = false;
        if (species_labels.empty() ||
            (int)species_labels.size() != builder_species_count ||
            species_labels_gen2 != builder_gen2) {
            species_labels.clear();
            species_labels_gen2 = builder_gen2;
            char raw[16];
            char label[32];
            for (int i = 1; i <= builder_species_count; i++) {
                bool ok = builder_gen2
                    ? gb_mock_gen2_species_name(ctx, i, raw, sizeof(raw))
                    : gb_mock_gen1_species_name(ctx, i, raw, sizeof(raw));
                if (ok) {
                    snprintf(label, sizeof(label), "%03d %s", i, raw);
                } else {
                    snprintf(label, sizeof(label), "%03d ???", i);
                }
                species_labels.emplace_back(label);
            }
        }

        int sel_idx = g_builder_species - 1;
        if (sel_idx < 0) sel_idx = 0;
        if (sel_idx >= (int)species_labels.size()) sel_idx = 0;
        if (ImGui::BeginCombo("Species##builder",
                              species_labels[sel_idx].c_str())) {
            for (int i = 0; i < (int)species_labels.size(); i++) {
                bool selected = (i == sel_idx);
                if (ImGui::Selectable(species_labels[i].c_str(), selected)) {
                    g_builder_species = i + 1;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SliderInt("Level##builder", &g_builder_level, 2, 100,
                         "%d", ImGuiSliderFlags_NoInput);
        ImGui::Checkbox("Shiny##builder", &g_builder_shiny);
        if (ImGui::Button("Send Pokemon##builder")) {
            bool ok = builder_gen2
                ? gb_mock_gen2_inject_builder(ctx,
                      g_builder_species, g_builder_level, g_builder_shiny)
                : gb_mock_gen1_inject_builder(ctx,
                      g_builder_species, g_builder_level, g_builder_shiny);
            if (ok) {
                g_builder_last_msg = "Sent - check your party.";
            } else {
                g_builder_last_msg = "Party full (6/6) - release or store a Pokemon first.";
            }
        }
        if (g_builder_last_msg) {
            ImGui::TextDisabled("%s", g_builder_last_msg);
        } else if (builder_gen1) {
            ImGui::TextDisabled("Moves/stats auto-filled from cart data. Shiny sets Gen-2-compatible DVs - Gen 1 itself doesn't display shinies, but Time Capsule trades to Gen 2 will.");
        } else {
            ImGui::TextDisabled("Moves and stats are auto-filled from the cart's own data tables. OT becomes the player.");
        }

        /* Distributions — scan injects/<game_id>/ for .pkm/.pk1/.pk2 files. */
        ImGui::Spacing();
        ImGui::TextDisabled("Distributions");
        if (g_event_files_count < 0) {
            g_event_files_count = gb_inject_file_scan(
                ctx, POKEMON_GAME_ID,
                g_event_files, GB_INJECT_FILE_MAX);
        }
        if (g_event_files_count == 0) {
            ImGui::TextDisabled(
                "No distribution files in injects/%s/.\n"
                "Drop .pkm / .pk1 / .pk2 files there and relaunch.",
                POKEMON_GAME_ID);
        } else {
            if (g_event_sel_idx < 0) g_event_sel_idx = 0;
            if (g_event_sel_idx >= g_event_files_count) g_event_sel_idx = 0;
            if (ImGui::BeginCombo("Distribution##event_pick",
                                  g_event_files[g_event_sel_idx].display)) {
                for (int i = 0; i < g_event_files_count; i++) {
                    bool selected = (i == g_event_sel_idx);
                    ImGui::PushID(i);
                    if (ImGui::Selectable(g_event_files[i].display, selected)) {
                        g_event_sel_idx = i;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            char details[512];
            if (gb_inject_file_describe(ctx,
                                        &g_event_files[g_event_sel_idx],
                                        details, sizeof(details))) {
                ImGui::Separator();
                ImGui::TextUnformatted(details);
                ImGui::Spacing();
            }
            if (ImGui::Button("Send Pokemon##event_apply")) {
                char err[128];
                if (gb_inject_file_apply(ctx,
                                         &g_event_files[g_event_sel_idx],
                                         err, sizeof(err))) {
                    snprintf(g_event_last_msg, sizeof(g_event_last_msg),
                             "Applied %s - check your party.",
                             g_event_files[g_event_sel_idx].filename);
                } else {
                    snprintf(g_event_last_msg, sizeof(g_event_last_msg),
                             "Apply failed: %s", err);
                }
            }
            if (g_event_last_msg[0]) {
                ImGui::TextDisabled("%s", g_event_last_msg);
            }
        }

        /* Trade Evolutions → Level Evolutions: patches the cart's evos_moves
         * rows so trade evolvers fire on level-up instead. */
        ImGui::Spacing();
        ImGui::TextDisabled("Trade Evolutions");
        if (gb_evolve_patch_is_supported(ctx)) {
            bool on = gb_evolve_patch_is_enabled();
            if (ImGui::Checkbox("Enable level-up evolutions for trade-only Pokemon", &on)) {
                gb_evolve_patch_set_enabled(ctx, on);
            }
            ImGui::TextDisabled(
                "Replaces EV_TRADE rows in cart ROM with EV_LEVEL at "
                "sensible thresholds. A Pokemon already past its new "
                "threshold evolves on the next level-up event (battle "
                "XP or Rare Candy), not retroactively.");
            const GBEvolvePatchEntry* entries = NULL;
            int n = gb_evolve_patch_list(ctx, &entries);
            for (int i = 0; i < n; i++) {
                ImGui::BulletText("%s", entries[i].label);
            }
        } else {
            ImGui::TextDisabled("(not a Pokemon cart)");
        }

        /* Wild Exclusives 50/50 — each version-exclusive encounter slot rolls
         * a coin to pick across all versions' species. Fresh mix per toggle. */
        ImGui::Spacing();
        ImGui::TextDisabled("Wild Exclusives");
        if (gb_wild_5050_is_supported(ctx)) {
            bool on = gb_wild_5050_is_enabled();
            if (ImGui::Checkbox("Mix in other versions' wild exclusives", &on)) {
                gb_wild_5050_set_enabled(ctx, on);
            }
            ImGui::TextDisabled(
                "Per-slot random pick across the union of all three "
                "versions' species at that route position (%d slots "
                "for this cart). The mix re-rolls every time you "
                "walk into a new map.",
                gb_wild_5050_slot_count(ctx));
        } else {
            ImGui::TextDisabled("(Pokemon cart only)");
        }

        /* Money. Both gens store it as 3-byte BCD, maxing at 999,999. */
        static int g_money_input = 999999;
        ImGui::Spacing();
        ImGui::TextDisabled("Money");
        int current_money = builder_gen2
            ? gb_mock_gen2_get_money(ctx)
            : gb_mock_gen1_get_money(ctx);
        ImGui::Text("Current: %d", current_money >= 0 ? current_money : 0);
        ImGui::InputInt("Amount##money", &g_money_input);
        if (g_money_input < 0)      g_money_input = 0;
        if (g_money_input > 999999) g_money_input = 999999;
        ImGui::SameLine();
        if (ImGui::Button("Set##money")) {
            if (builder_gen2)
                gb_mock_gen2_set_money(ctx, g_money_input);
            else
                gb_mock_gen1_set_money(ctx, g_money_input);
        }
        ImGui::SameLine();
        if (ImGui::Button("Max##money")) {
            g_money_input = 999999;
            if (builder_gen2)
                gb_mock_gen2_set_money(ctx, 999999);
            else
                gb_mock_gen1_set_money(ctx, 999999);
        }

        /* Give Item — writes the chosen item directly into the player's bag.
         * Dropdown reads names from the cart's own ItemNames table. */
        static int  g_give_item_id = 0;     /* 1-based; 0 = unset */
        static int  g_give_item_qty = 99;
        static char g_give_item_msg[120] = "";
        ImGui::Spacing();
        ImGui::TextDisabled("Give Item");
        {
            static int g_item_list_built_for_gen = 0;
            static int g_item_ids[256];
            static char g_item_labels[256][24];
            static int g_item_label_count = 0;
            int gen = (builder_gen2 ? 2 : 1);
            if (g_item_list_built_for_gen != gen) {
                g_item_label_count = 0;
                char nm[20];
                int total = gen == 2
                    ? GB_MOCK_GEN2_ITEM_COUNT
                    : GB_MOCK_GEN1_ITEM_COUNT;
                for (int id = 1; id <= total &&
                     g_item_label_count < 256; id++) {
                    bool ok = (gen == 2)
                        ? gb_mock_gen2_item_name(ctx, id, nm, sizeof(nm))
                        : gb_mock_gen1_item_name(ctx, id, nm, sizeof(nm));
                    if (!ok) continue;
                    /* Hide un-tossable items (Gen 1 key items; Gen 2 key
                     * items + TMs/HMs which give_item doesn't route). */
                    if (gen == 1 &&
                        gb_mock_gen1_item_is_key(ctx, id))
                        continue;
                    if (gen == 2) {
                        GBGen2ItemPocket p =
                            gb_mock_gen2_item_pocket(ctx, id);
                        if (p != GB_GEN2_POCKET_ITEM &&
                            p != GB_GEN2_POCKET_BALL) continue;
                    }
                    g_item_ids[g_item_label_count] = id;
                    snprintf(g_item_labels[g_item_label_count],
                             sizeof(g_item_labels[0]),
                             "%03d - %s", id, nm);
                    g_item_label_count++;
                }
                g_item_list_built_for_gen = gen;
                if (g_give_item_id == 0 && g_item_label_count > 0)
                    g_give_item_id = g_item_ids[0];
            }

            int sel = 0;
            for (int i = 0; i < g_item_label_count; i++) {
                if (g_item_ids[i] == g_give_item_id) { sel = i; break; }
            }
            const char* current_label = (g_item_label_count > 0)
                ? g_item_labels[sel] : "(no items)";
            if (ImGui::BeginCombo("Item##give_item", current_label)) {
                for (int i = 0; i < g_item_label_count; i++) {
                    bool selected = (i == sel);
                    ImGui::PushID(i);
                    if (ImGui::Selectable(g_item_labels[i], selected)) {
                        g_give_item_id = g_item_ids[i];
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::SliderInt("Quantity##give_item", &g_give_item_qty, 1, 99);
            if (ImGui::Button("Give##give_item")) {
                bool ok;
                if (builder_gen2) {
                    ok = gb_mock_gen2_give_item(ctx,
                               g_give_item_id, g_give_item_qty);
                } else {
                    ok = gb_mock_gen1_give_item(ctx,
                               g_give_item_id, g_give_item_qty);
                }
                if (ok) {
                    snprintf(g_give_item_msg, sizeof(g_give_item_msg),
                             "Gave %d x item #%d - check your bag.",
                             g_give_item_qty, g_give_item_id);
                } else {
                    snprintf(g_give_item_msg, sizeof(g_give_item_msg),
                             "Failed: bag/pocket full (cart caps at "
                             "20 items / 12 balls). Toss one in-game "
                             "to make room, or pick an item you "
                             "already carry to stack quantity.");
                }
            }
            if (g_give_item_msg[0]) {
                ImGui::TextDisabled("%s", g_give_item_msg);
            }
            if (builder_gen2) {
                ImGui::TextDisabled(
                    "Items routed to Items / Balls pocket based on "
                    "the cart's ItemAttributes. Key Items and TMs/HMs "
                    "not yet supported.");
            }
        }

        ImGui::PopTextWrapPos();
    }
}
