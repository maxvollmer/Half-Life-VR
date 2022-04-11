
#include "VRCEGUIWrapper.h"

#define CEGUI_USE_GLEW
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include <CEGUI/RendererModules/OpenGL/GLRenderer.h>
#include <CEGUI/System.h>
#include <CEGUI/AnimationManager.h>
#include <CEGUI/SchemeManager.h>
#include <CEGUI/falagard/WidgetLookManager.h>
#include <CEGUI/FontManager.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/ScriptModule.h>
#include <CEGUI/XMLParser.h>
#include <CEGUI/DefaultResourceProvider.h>
#include <CEGUI/Window.h>
#include <CEGUI/widgets/FrameWindow.h>
#include <CEGUI/RendererModules/OpenGL/Texture.h>

#include <iostream>
#include <filesystem>

namespace
{
	static CEGUI::OpenGLRenderer* gCEGUIRenderer = nullptr;
	//static CEGUI::TextureTarget* gCEGUIRenderTextureTarget = nullptr;
	//static CEGUI::GUIContext* gCEGUIRenderGuiContext = nullptr;
}

void VRCEGUIWrapper::InitCEGUI()
{
	if (gCEGUIRenderer != nullptr)
	{
		DestroyCEGUI();
	}

	try
	{
		int width = 256;
		int height = 256;
		CEGUI::Sizef size(static_cast<float>(width), static_cast<float>(height));

		gCEGUIRenderer = &CEGUI::OpenGLRenderer::bootstrapSystem(CEGUI::OpenGLRenderer::TextureTargetType::TTT_PBUFFER);
		//gCEGUIRenderTextureTarget = gCEGUIRenderer->createTextureTarget();
		//gCEGUIRenderTextureTarget->declareRenderSize(size);
		//gCEGUIRenderGuiContext = &CEGUI::System::getSingleton().createGUIContext(*gCEGUIRenderTextureTarget);

		CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());
		rp->setResourceGroupDirectory("animations", "./cegui/datafiles/animations/");
		rp->setResourceGroupDirectory("fonts", "./cegui/datafiles/fonts/");
		rp->setResourceGroupDirectory("imagesets", "./cegui/datafiles/imagesets/");
		rp->setResourceGroupDirectory("layouts", "./cegui/datafiles/layouts/");
		rp->setResourceGroupDirectory("looknfeel", "./cegui/datafiles/looknfeel/");
		rp->setResourceGroupDirectory("lua_scripts", "./cegui/datafiles/lua_scripts/");
		rp->setResourceGroupDirectory("schemes", "./cegui/datafiles/schemes/");
		rp->setResourceGroupDirectory("schemas", "./cegui/datafiles/xml_schemas/");

		CEGUI::AnimationManager::setDefaultResourceGroup("animations");
		CEGUI::Font::setDefaultResourceGroup("fonts");
		CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
		CEGUI::WindowManager::setDefaultResourceGroup("layouts");
		CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeel");
		CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
		CEGUI::Scheme::setDefaultResourceGroup("schemes");
		CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
		if (parser != nullptr && parser->isPropertyPresent("SchemaDefaultResourceGroup"))
		{
			parser->setProperty("SchemaDefaultResourceGroup", "schemas");
		}

		CEGUI::SchemeManager::getSingleton().createFromFile("TaharezLook.scheme");

		CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-14.font");
		CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultFont("DejaVuSans-14");

		CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");

		CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultTooltipType("TaharezLook/Tooltip");

		CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
		CEGUI::Window* myRoot = wmgr.createWindow("DefaultWindow", "root");
		CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(myRoot);

		CEGUI::FrameWindow* fWnd = static_cast<CEGUI::FrameWindow*>(wmgr.createWindow("TaharezLook/FrameWindow", "testWindow"));
		myRoot->addChild(fWnd);

		fWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(0.25f, 0.f), CEGUI::UDim(0.25f, 0.f)));
		fWnd->setSize(CEGUI::USize(CEGUI::UDim(0.5f, 0.f), CEGUI::UDim(0.5f, 0.f)));

		fWnd->setText("Hello World!");
	}
	catch (CEGUI::Exception& e)
	{
		std::stringstream ss;
		ss << "Caught " << e.getName() << " \"" << e.getMessage() << "\" in " << e.getFunctionName() << "(" << e.getFileName() << ", line " << e.getLine() << ")";

		std::string bla = ss.str();
		std::cout << bla << std::endl;
	}
}

void VRCEGUIWrapper::DestroyCEGUI()
{
	if (gCEGUIRenderer != nullptr)
	{
		CEGUI::System::destroy();
		CEGUI::OpenGLRenderer::destroy(*gCEGUIRenderer);
	}

	gCEGUIRenderer = nullptr;
	//gCEGUIRenderTextureTarget = nullptr;
	//gCEGUIRenderGuiContext = nullptr;
}

void VRCEGUIWrapper::DoTheThing()
{
	CEGUI::System::getSingleton().renderAllGUIContexts();

	/*
	gCEGUIRenderer->beginRendering();
	gCEGUIRenderTextureTarget->clear();
	gCEGUIRenderGuiContext->draw();
	gCEGUIRenderer->endRendering();
	*/
}

unsigned int VRCEGUIWrapper::GetGUITextureID()
{
	//CEGUI::OpenGLTexture& glTexture = static_cast<CEGUI::OpenGLTexture&>(gCEGUIRenderTextureTarget->getTexture());
	return 0;// glTexture.getOpenGLTexture();
}

unsigned int TEMPVRGetGetGUITextureID()
{
	return VRCEGUIWrapper::GetGUITextureID();
}



#undef _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#undef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
