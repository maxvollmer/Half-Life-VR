
#include "vr_gl.h"

#include "VRCommon.h"
#include "VRGUIRenderer.h"
#include "VRTextureHelper.h"
#include "VRGameFunctions.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_IMPLEMENTATION

#pragma warning(push) 
#pragma warning(disable: 4996)  // vsprintf and fopen used by nuklear.h give deprecation warning
#include "nuklear.h"
#pragma warning(pop)

namespace
{
    // would be nicer to have these as a members of VRGUIRenderer,
    // but with HL's include hell, nopenopenope
    struct nk_context ctx;
    struct nk_buffer cmds;
    struct nk_draw_null_texture nulltexture;
    struct nk_font_atlas atlas;
    unsigned int font_tex;
    struct nk_font* font_heading = nullptr;
    struct nk_font* font_text = nullptr;
    struct nk_font* font_text_smol = nullptr;

    struct nk_ogl_vertex
    {
        float position[2];
        float uv[2];
        nk_byte col[4];
    };

    void SafeSetFont(nk_font* font)
    {
        if (font)
        {
            nk_style_set_font(&ctx, &font->handle);
        }
        else if (font_text)
        {
            nk_style_set_font(&ctx, &font_text->handle);
        }
        else if (atlas.default_font)
        {
            nk_style_set_font(&ctx, &atlas.default_font->handle);
        }
        else
        {
            VRGameFunctions::PrintToConsole("SetFont failed: No font!\n");
        }
    }

// TODO: Make menu colors fit HL:VR
#define HLVR_COLOR_MAP(NK_COLOR)\
    NK_COLOR(NK_COLOR_TEXT,                     175,175,175,255) \
    NK_COLOR(NK_COLOR_WINDOW,                   45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_HEADER,                   40, 40, 40, 255) \
    NK_COLOR(NK_COLOR_BORDER,                   65, 65, 65, 255) \
    NK_COLOR(NK_COLOR_BUTTON,                   50, 50, 50, 255) \
    NK_COLOR(NK_COLOR_BUTTON_HOVER,             40, 40, 40, 255) \
    NK_COLOR(NK_COLOR_BUTTON_ACTIVE,            35, 35, 35, 255) \
    NK_COLOR(NK_COLOR_TOGGLE,                   100,100,100,255) \
    NK_COLOR(NK_COLOR_TOGGLE_HOVER,             120,120,120,255) \
    NK_COLOR(NK_COLOR_TOGGLE_CURSOR,            45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_SELECT,                   45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_SELECT_ACTIVE,            35, 35, 35,255) \
    NK_COLOR(NK_COLOR_SLIDER,                   38, 38, 38, 255) \
    NK_COLOR(NK_COLOR_SLIDER_CURSOR,            100,100,100,255) \
    NK_COLOR(NK_COLOR_SLIDER_CURSOR_HOVER,      120,120,120,255) \
    NK_COLOR(NK_COLOR_SLIDER_CURSOR_ACTIVE,     150,150,150,255) \
    NK_COLOR(NK_COLOR_PROPERTY,                 38, 38, 38, 255) \
    NK_COLOR(NK_COLOR_EDIT,                     38, 38, 38, 255)  \
    NK_COLOR(NK_COLOR_EDIT_CURSOR,              175,175,175,255) \
    NK_COLOR(NK_COLOR_COMBO,                    45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_CHART,                    120,120,120,255) \
    NK_COLOR(NK_COLOR_CHART_COLOR,              45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_CHART_COLOR_HIGHLIGHT,    255, 0,  0, 255) \
    NK_COLOR(NK_COLOR_SCROLLBAR,                40, 40, 40, 255) \
    NK_COLOR(NK_COLOR_SCROLLBAR_CURSOR,         100,100,100,255) \
    NK_COLOR(NK_COLOR_SCROLLBAR_CURSOR_HOVER,   120,120,120,255) \
    NK_COLOR(NK_COLOR_SCROLLBAR_CURSOR_ACTIVE,  150,150,150,255) \
    NK_COLOR(NK_COLOR_TAB_HEADER,               40, 40, 40,255)
    const struct nk_color
        hlvr_color_style[NK_COLOR_COUNT] = {
        #define NK_COLOR(a,b,c,d,e) {b,c,d,e},
            HLVR_COLOR_MAP(NK_COLOR)
        #undef NK_COLOR
    };
#undef HLVR_COLOR_MAP
}

VRGUIRenderer VRGUIRenderer::instance;
VRGUIRenderer& VRGUIRenderer::Instance()
{
    return VRGUIRenderer::instance;
}

void VRGUIRenderer::Init(int guiWidth, int guiHeight)
{
    if (isInitialized)
    {
        ShutDown();
    }

    // GUI is designed for a 2048x2048 square, scale all size/widths/etc accordingly.
    m_guiWidth = guiWidth;
    m_guiHeight = guiHeight;
    m_guiScaleW = guiWidth / 2048.f;
    m_guiScaleH = guiHeight / 2048.f;

    nulltexture.texture.id = VRTextureHelper::Instance().GetTexture("null.png");
    nulltexture.uv = { 0.f, 0.f };

    nk_font_atlas_init_default(&atlas);
    nk_font_atlas_begin(&atlas);

    std::string font_path = GetPathFor("/fonts/Roboto-Regular.ttf").u8string();

    font_heading = nk_font_atlas_add_from_file(&atlas, font_path.c_str(), 60 * m_guiScaleH, 0);
    font_text = nk_font_atlas_add_from_file(&atlas, font_path.c_str(), 48 * m_guiScaleH, 0);
    font_text_smol = nk_font_atlas_add_from_file(&atlas, font_path.c_str(), 36 * m_guiScaleH, 0);

    if (!font_heading || !font_text || !font_text_smol)
    {
        VRGameFunctions::PrintToConsole("SetFont failed: Failed to load font!\n");
    }

    int width, height;
    const void* image = nk_font_atlas_bake(&atlas, &width, &height, NK_FONT_ATLAS_RGBA32);

    glGenTextures(1, &font_tex);
    glBindTexture(GL_TEXTURE_2D, font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    nk_font_atlas_end(&atlas, nk_handle_id((int)font_tex), &nulltexture);

    nk_init_default(&ctx, font_text ? &font_text->handle : nullptr);
    nk_style_from_table(&ctx, hlvr_color_style);
    nk_buffer_init_default(&cmds);

    ctx.style.window.header.label_padding = nk_vec2(20, 20);
    ctx.style.window.header.padding = nk_vec2(20, 20);
    ctx.style.window.header.spacing = nk_vec2(20, 20);

    nk_style_load_all_cursors(&ctx, atlas.cursors);

    SafeSetFont(font_text);

    isInitialized = true;
}

void VRGUIRenderer::ShutDown()
{
    if (isInitialized)
    {
        nk_free(&ctx);
        isInitialized = false;
    }
}

void VRGUIRenderer::UpdateControllerState(VRControllerState& state)
{
    lastState = currentState;
    currentState = state;
}

void VRGUIRenderer::UpdateGUI(int guiWidth, int guiHeight, bool isInGame)
{
    if (!isInitialized || guiWidth != m_guiWidth || guiHeight != m_guiHeight)
    {
        Init(guiWidth, guiHeight);
    }

    if (!ctx.style.font)
    {
        if (atlas.default_font)
        {
            VRGameFunctions::PrintToConsole("UpdateGUI: ctx.style.font was null, falling back to default!\n");
            ctx.style.font = &atlas.default_font->handle;
        }
        else
        {
            VRGameFunctions::PrintToConsole("UpdateGUI failed: ctx.style.font is null and no default font!\n");
            return;
        }
    }

    UpdateVRCursor();

    UpdateGUIElements(isInGame);

    DrawGUI();

    nk_clear(&ctx);
}

void VRGUIRenderer::UpdateVRCursor()
{
    nk_input_begin(&ctx);
    if (currentState.isOverGUI)
    {
        int x = (int)(currentState.x * m_guiWidth);
        int y = (int)(currentState.y * m_guiHeight);
        nk_input_motion(&ctx, x, y);
        nk_input_button(&ctx, nk_buttons::NK_BUTTON_LEFT, x, y, currentState.isPressed);
    }
    else
    {
        nk_input_motion(&ctx, -100, -100);
        nk_input_button(&ctx, nk_buttons::NK_BUTTON_LEFT, -100, -100, false);
    }
    nk_input_end(&ctx);
}

void VRGUIRenderer::UpdateGUIElements(bool isInGame)
{
    static bool isInNewGameMenu = false;
    static int skill = 2;
    static nk_size volume = 100;

    if (nk_begin(&ctx, "HLVRMainMenu", nk_rect(0, 0, (float)m_guiWidth, (float)m_guiHeight), NK_WINDOW_NO_SCROLLBAR))
    {
        SafeSetFont(font_heading);
        nk_layout_row_begin(&ctx, NK_STATIC, 0, 7);
        {
            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 1600 * m_guiScaleW);
            nk_label(&ctx, "Welcome to Half-Life: VR!", NK_TEXT_LEFT);
        }
        nk_layout_row_end(&ctx);

        SafeSetFont(font_text);
        nk_layout_row_begin(&ctx, NK_STATIC, 0, 7);
        {
            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 250 * m_guiScaleW);
            if (nk_option_label(&ctx, "Easy", skill == 1))
            {
                skill = 1;
                VRGameFunctions::SetSkill(skill);
            }
            nk_layout_row_push(&ctx, 250 * m_guiScaleW);
            if (nk_option_label(&ctx, "Normal", skill == 2))
            {
                skill = 2;
                VRGameFunctions::SetSkill(skill);
            }
            nk_layout_row_push(&ctx, 250 * m_guiScaleW);
            if (nk_option_label(&ctx, "Hard", skill == 3))
            {
                skill = 3;
                VRGameFunctions::SetSkill(skill);
            }

            nk_layout_row_push(&ctx, 200 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 200 * m_guiScaleW);
            nk_label(&ctx, "Volume:", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 600 * m_guiScaleW);
            if (nk_progress(&ctx, &volume, 100, true))
            {
                VRGameFunctions::SetVolume(volume / 100.f);
            }
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_dynamic(&ctx, 100 * m_guiScaleH, 1);
        nk_label_wrap(&ctx, " ");

        if (isInNewGameMenu)
        {
            nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                nk_layout_row_push(&ctx, 855 * m_guiScaleW);
                if (nk_button_label(&ctx, "Intro Train Ride"))
                {
                    VRGameFunctions::StartNewGame(false);
                    isInNewGameMenu = false;
                }
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_dynamic(&ctx, 30 * m_guiScaleH, 1);
            nk_label_wrap(&ctx, " ");

            nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                nk_layout_row_push(&ctx, 855 * m_guiScaleW);
                if (nk_button_label(&ctx, "Straight to Black Mesa Inbound"))
                {
                    VRGameFunctions::StartNewGame(true);
                    isInNewGameMenu = false;
                }
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_dynamic(&ctx, 30 * m_guiScaleH, 1);
            nk_label_wrap(&ctx, " ");

            nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                nk_layout_row_push(&ctx, 855 * m_guiScaleW);
                if (nk_button_label(&ctx, "Hazard Course (Training)"))
                {
                    VRGameFunctions::StartHazardCourse();
                    isInNewGameMenu = false;
                }
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_dynamic(&ctx, 30 * m_guiScaleH, 1);
            nk_label_wrap(&ctx, " ");

            nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                nk_layout_row_push(&ctx, 855 * m_guiScaleW);
                if (nk_button_label(&ctx, "Nevermind (Back)"))
                {
                    isInNewGameMenu = false;
                }
            }
            nk_layout_row_end(&ctx);
        }
        else
        {
            bool canContinue = isInGame || VRGameFunctions::QuickSaveExists();

            nk_layout_row_begin(&ctx, NK_STATIC, 0, canContinue ? 4 : 3);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                nk_layout_row_push(&ctx, 400 * m_guiScaleW);
                if (nk_button_label(&ctx, "New Game"))
                {
                    isInNewGameMenu = true;
                }

                nk_layout_row_push(&ctx, 20 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                if (canContinue)
                {
                    nk_layout_row_push(&ctx, 400 * m_guiScaleW);
                    if (nk_button_label(&ctx, "Continue"))
                    {
                        if (isInGame)
                        {
                            VRGameFunctions::ToggleMenu();
                        }
                        else
                        {
                            VRGameFunctions::QuickLoad();
                        }
                    }
                }
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_dynamic(&ctx, 30 * m_guiScaleH, 1);
            nk_label_wrap(&ctx, " ");

            bool canQuickLoad = VRGameFunctions::QuickSaveExists();
            nk_layout_row_begin(&ctx, NK_STATIC, 0, canQuickLoad ? 4 : 3);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                if (isInGame)
                {
                    nk_layout_row_push(&ctx, 400 * m_guiScaleW);
                    if (nk_button_label(&ctx, "Quick Save"))
                    {
                        VRGameFunctions::QuickSave();
                    }

                    nk_layout_row_push(&ctx, 20 * m_guiScaleW);
                    nk_label(&ctx, " ", NK_TEXT_LEFT);

                    if (canQuickLoad)
                    {
                        nk_layout_row_push(&ctx, 400 * m_guiScaleW);
                        if (nk_button_label(&ctx, "Quick Load"))
                        {
                            VRGameFunctions::QuickLoad();
                        }
                    }
                }
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_dynamic(&ctx, 30 * m_guiScaleH, 1);
            nk_label_wrap(&ctx, " ");

            nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                nk_layout_row_push(&ctx, 855 * m_guiScaleW);
                if (nk_button_label(&ctx, "Quit Game (Does Not Auto Save!)"))
                {
                    VRGameFunctions::QuitGame();
                }
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
            {
                nk_layout_row_push(&ctx, 50 * m_guiScaleW);
                nk_label(&ctx, " ", NK_TEXT_LEFT);

                nk_layout_row_push(&ctx, 0 * m_guiScaleW);
                nk_button_label(&ctx, " ");
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_dynamic(&ctx, 30 * m_guiScaleH, 1);
            nk_label_wrap(&ctx, " ");
        }

        SafeSetFont(font_heading);
        nk_layout_row_dynamic(&ctx, 200 * m_guiScaleH, 1);
        nk_label_wrap(&ctx, " ");

        SafeSetFont(font_text_smol);
        nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
        {
            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 1600 * m_guiScaleW);
            nk_label_wrap(&ctx, "This is a minimal VR menu added for your convenience. A more powerful menu is in the works. To access the console, load savegames, or change settings use the desktop game window or HLVRConfig.");
        }
        nk_layout_row_end(&ctx);

        SafeSetFont(font_text_smol);
        nk_layout_row_begin(&ctx, NK_STATIC, 0, 2);
        {
            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 1600 * m_guiScaleW);
            nk_label_wrap(&ctx, "Half-Life: VR is made by Max Vollmer with the help of Rorin, Formic.Sapien, DoomMarine23, and Junt. Not affiliated with Valve.");
        }
        nk_layout_row_end(&ctx);

        nk_layout_row_begin(&ctx, NK_STATIC, 50 * m_guiScaleH, 2);
        {
            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 1600 * m_guiScaleW);
            nk_label_wrap(&ctx, "halflifevr.de");

            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 1600 * m_guiScaleW);
            nk_label_wrap(&ctx, "maxmakesmods.de");

            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 1600 * m_guiScaleW);
            nk_label_wrap(&ctx, "youtube.com/maxmakesmods");

            nk_layout_row_push(&ctx, 50 * m_guiScaleW);
            nk_label(&ctx, " ", NK_TEXT_LEFT);

            nk_layout_row_push(&ctx, 1600 * m_guiScaleW);
            nk_label_wrap(&ctx, "discord.gg/jujwEGf62K or maxmakesmods.de/discord");
        }
    }
    nk_end(&ctx);
}

// Adapted from nuklear_sdl_gl2.h
void VRGUIRenderer::DrawGUI()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, m_guiWidth, m_guiHeight, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    {
        /* fill converting configuration */
        static const struct nk_draw_vertex_layout_element vertex_layout[] =
        {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_ogl_vertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_ogl_vertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_ogl_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };

        struct nk_convert_config config;
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_ogl_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_ogl_vertex);
        config.null = nulltexture;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;

        /* convert shapes into vertexes */
        struct nk_buffer vbuf, ebuf;
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&ctx, &cmds, &vbuf, &ebuf, &config);

        /* setup vertex buffer pointer */
        {
            GLsizei vs = sizeof(struct nk_ogl_vertex);
            size_t vp = offsetof(struct nk_ogl_vertex, position);
            size_t vt = offsetof(struct nk_ogl_vertex, uv);
            size_t vc = offsetof(struct nk_ogl_vertex, col);
            const void* vertices = nk_buffer_memory_const(&vbuf);
            glVertexPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vp));
            glTexCoordPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vt));
            glColorPointer(4, GL_UNSIGNED_BYTE, vs, (const void*)((const nk_byte*)vertices + vc));
        }

        /* iterate over and execute each draw command */
        const nk_draw_index* offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);
        const struct nk_draw_command* cmd;
        nk_draw_foreach(cmd, &ctx, &cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x), (GLint)(m_guiHeight - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)), (GLint)(cmd->clip_rect.w), (GLint)(cmd->clip_rect.h));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&ctx);
        nk_buffer_clear(&cmds);
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}
