
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

    void SetFont(nk_font* font)
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

void VRGUIRenderer::Init()
{
    if (isInitialized)
    {
        ShutDown();
    }

    nulltexture.texture.id = VRTextureHelper::Instance().GetTexture("null.png");
    nulltexture.uv = { 0.f, 0.f };

    nk_font_atlas_init_default(&atlas);
    nk_font_atlas_begin(&atlas);

    std::string font_path = GetPathFor("/fonts/Roboto-Regular.ttf").u8string();

    font_heading = nk_font_atlas_add_from_file(&atlas, font_path.c_str(), 60, 0);
    font_text = nk_font_atlas_add_from_file(&atlas, font_path.c_str(), 48, 0);
    font_text_smol = nk_font_atlas_add_from_file(&atlas, font_path.c_str(), 36, 0);

    if (!font_heading || !font_text || !font_text_smol)
    {
        // TODO: Print warning
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

    SetFont(font_text);

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

void VRGUIRenderer::UpdateGUI(bool isInGame)
{
    if (!isInitialized)
    {
        Init();
    }

    if (!ctx.style.font)
    {
        if (atlas.default_font)
        {
            // TODO: Print warning
            ctx.style.font = &atlas.default_font->handle;
        }
        else
        {
            // TODO: Print error!
            return;
        }
    }

    //lastState = currentState;
    //currentState = state;
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
        int x = (int)(currentState.x * GuiWidth);
        int y = (int)(currentState.y * GuiHeight);
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
    static int i = 20;

    if (nk_begin(&ctx, "HLVRMainMenu", nk_rect(0, 0, GuiWidth, GuiHeight), NK_WINDOW_NO_SCROLLBAR))
    {
        SetFont(font_heading);
        nk_layout_row_dynamic(&ctx, 0, 1);
        {
            nk_label(&ctx, "Welcome to Half-Life: VR!", NK_TEXT_LEFT);
        }

        SetFont(font_text_smol);
        nk_layout_row_dynamic(&ctx, 0, 1);
        {
            nk_label_wrap(&ctx, "This is a minimal VR menu added for your convenience. A more powerful menu is in the works.");
            nk_label_wrap(&ctx, "To access the console, load savegames, or change settings use the desktop game window or HLVRConfig.");
        }

        SetFont(font_text);
        nk_layout_row_begin(&ctx, NK_STATIC, 0, 5);
        {
            nk_layout_row_push(&ctx, 300);
            if (nk_option_label(&ctx, "Easy", skill == 1))
            {
                skill = 1;
                VRGameFunctions::SetSkill(skill);
            }
            nk_layout_row_push(&ctx, 300);
            if (nk_option_label(&ctx, "Normal", skill == 2))
            {
                skill = 2;
                VRGameFunctions::SetSkill(skill);
            }
            nk_layout_row_push(&ctx, 300);
            if (nk_option_label(&ctx, "Hard", skill == 3))
            {
                skill = 3;
                VRGameFunctions::SetSkill(skill);
            }

            nk_layout_row_push(&ctx, 300);
            nk_label(&ctx, "Volume:", NK_TEXT_LEFT);
            nk_layout_row_push(&ctx, 500);
            if (nk_progress(&ctx, &volume, 100, true))
            {
                VRGameFunctions::SetVolume(volume / 100.f);
            }
        }
        nk_layout_row_end(&ctx);

        if (isInNewGameMenu)
        {
            nk_layout_row_dynamic(&ctx, 0, 4);
            {
                if (nk_button_label(&ctx, "Intro Train Ride"))
                {
                    VRGameFunctions::StartNewGame(false);
                    isInNewGameMenu = false;
                }
                if (nk_button_label(&ctx, "Straight to Black Mesa Inbound"))
                {
                    VRGameFunctions::StartNewGame(true);
                    isInNewGameMenu = false;
                }
                if (nk_button_label(&ctx, "Hazard Course (Training)"))
                {
                    VRGameFunctions::StartHazardCourse();
                }
                if (nk_button_label(&ctx, "Nevermind (Back)"))
                {
                    isInNewGameMenu = false;
                }
            }
        }
        else
        {
            bool canContinue = isInGame || VRGameFunctions::QuickSaveExists();

            nk_layout_row_begin(&ctx, NK_STATIC, 0, canContinue ? 2 : 1);
            {
                nk_layout_row_push(&ctx, 400);
                if (nk_button_label(&ctx, "New Game"))
                {
                    isInNewGameMenu = true;
                }

                if (canContinue)
                {
                    nk_layout_row_push(&ctx, 400);
                    if (nk_button_label(&ctx, "Continue"))
                    {
                        if (isInGame)
                        {
                            VRGameFunctions::CloseMenu();
                        }
                        else
                        {
                            VRGameFunctions::QuickLoad();
                        }
                    }
                }
            }
            nk_layout_row_end(&ctx);

            bool canQuickLoad = VRGameFunctions::QuickSaveExists();
            nk_layout_row_begin(&ctx, NK_STATIC, 0, canQuickLoad ? 2 : 1);
            {
                if (isInGame)
                {
                    nk_layout_row_push(&ctx, 400);
                    if (nk_button_label(&ctx, "Quick Save"))
                    {
                        VRGameFunctions::QuickSave();
                    }

                    if (canQuickLoad)
                    {
                        nk_layout_row_push(&ctx, 400);
                        if (nk_button_label(&ctx, "Quick Load"))
                        {
                            VRGameFunctions::QuickLoad();
                        }
                    }
                }
            }
            nk_layout_row_end(&ctx);

            nk_layout_row_static(&ctx, 0, 400, 1);
            if (nk_button_label(&ctx, "Quit Game (Does Not Auto Save!)"))
            {
                VRGameFunctions::QuitGame();
            }
        }

        SetFont(font_heading);
        nk_layout_row_dynamic(&ctx, 100, 1);
        nk_label_wrap(&ctx, " ");

        SetFont(font_text_smol);
        nk_layout_row_dynamic(&ctx, 0, 1);
        {
            nk_label_wrap(&ctx, "Half-Life: VR is made by Max Vollmer with the help of Rorin, Formic.Sapien, DoomMarine23, and Junt.");
        }

        nk_layout_row_static(&ctx, 0, 400, 4);
        {
            // TODO: "start" is Windows. If I (or someone else) ever get around to adding support
            // for other systems, depending on the system the command is "xdg-open" or "open".
            if (nk_button_label(&ctx, "Join \"Max Makes Mods\" Discord"))
            {
                system("start https://discord.gg/jujwEGf62K");
            }
            if (nk_button_label(&ctx, "Support the Mod (Ko-Fi)"))
            {
                system("start https://www.ko-fi.com/maxmakesmods");
            }
            if (nk_button_label(&ctx, "Support the Mod (Patreon)"))
            {
                system("start https://www.patreon.com/maxmakesmods");
            }
            if (nk_button_label(&ctx, "Mod Website"))
            {
                system("start https://www.halflifevr.de");
            }
        }

    }
    nk_end(&ctx);
}

void VRGUIRenderer::DrawGUI()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glViewport(0, 0, (GLsizei)GuiWidth, (GLsizei)GuiHeight);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, GuiWidth, GuiHeight, 0.0f, -1.0f, 1.0f);
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
            glScissor((GLint)(cmd->clip_rect.x), (GLint)(GuiHeight - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)), (GLint)(cmd->clip_rect.w), (GLint)(cmd->clip_rect.h));
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

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    glPopAttrib();
}
