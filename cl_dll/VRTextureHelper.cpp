
#include <filesystem>

#include "hud.h"
#include "cl_util.h"
#include "VRInput.h"
#include "eiface.h"

#include "VRTextureHelper.h"
#include "VRCommon.h"

#include "vr_gl.h"
#include "LodePNG/lodepng.h"

unsigned int VRTextureHelper::GetHDGameTexture(const std::string& name)
{
	return GetTexture("game/" + name + ".png");
}

unsigned int VRTextureHelper::GetSkyboxTexture(const std::string& name, int index)
{
	unsigned int texture = GetTexture("skybox/" + name + m_mapSkyboxIndices[index] + ".png");
	if (!texture && name != "desert")
	{
		gEngfuncs.Con_DPrintf("Skybox texture %s not found, falling back to desert.\n", (name + m_mapSkyboxIndices[index]).data());
		texture = GetTexture("skybox/desert" + m_mapSkyboxIndices[index] + ".png");
	}
	return texture;
}

unsigned int VRTextureHelper::GetHDSkyboxTexture(const std::string& name, int index)
{
	unsigned int texture = GetTexture("skybox/hd/" + name + m_mapSkyboxIndices[index] + ".png");
	if (!texture)
	{
		gEngfuncs.Con_DPrintf("HD skybox texture %s not found, falling back to SD.\n", (name + m_mapSkyboxIndices[index]).data());
		texture = GetSkyboxTexture(name, index);
	}
	return texture;
}

const char* VRTextureHelper::GetSkyboxNameFromMapName(const std::string& mapName)
{
	auto& skyboxName = m_mapSkyboxNames.find(mapName);
	if (skyboxName != m_mapSkyboxNames.end())
		return skyboxName->second.data();
	else
		return "desert";
}

unsigned int VRTextureHelper::GetTexture(const std::string& name)
{
	if (m_textures.count(name) > 0)
		return m_textures[name];

	GLuint texture{ 0 };

	std::filesystem::path texturePath = GetPathFor("/textures/" + name);
	if (std::filesystem::exists(texturePath))
	{
		std::vector<unsigned char> image;
		unsigned int width, height;
		unsigned int error = lodepng::decode(image, width, height, texturePath.string().data());
		if (error)
		{
			gEngfuncs.Con_DPrintf("Error (%i) trying to load texture %s: %s\n", error, name.data(), lodepng_error_text(error));
		}
		else if ((width & (width - 1)) || (height & (height - 1)))
		{
			gEngfuncs.Con_DPrintf("Invalid texture %s, width and height must be power of 2!\n", name.data());
		}
		else
		{
			// Now load it into OpenGL
			ClearGLErrors();
			try
			{
				TryGLCall(glActiveTexture, GL_TEXTURE0);
				TryGLCall(glGenTextures, 1, &texture);
				TryGLCall(glBindTexture, GL_TEXTURE_2D, texture);
				TryGLCall(glTexImage2D, GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
				TryGLCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				TryGLCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				TryGLCall(glGenerateMipmap, GL_TEXTURE_2D);
				TryGLCall(glBindTexture, GL_TEXTURE_2D, 0);
			}
			catch (const OGLErrorException& e)
			{
				gEngfuncs.Con_DPrintf("Couldn't create texture %s, error: %s\n", name.data(), e.what());
				texture = 0;
			}
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("Couldn't load texture %s, it doesn't exist.\n", name.data());
		texture = 0;
	}

	m_textures[name] = texture;
	return texture;
}

VRTextureHelper VRTextureHelper::instance{};

// from openvr docs: Order is Front, Back, Left, Right, Top, Bottom.
const std::array<std::string, 6> VRTextureHelper::m_mapSkyboxIndices{
	{
		{"ft"},
		{"bk"},
		{"lf"},
		{"rt"},
		{"up"},
		{"dn"}
	}
};

const std::unordered_map<std::string, std::string> VRTextureHelper::m_mapSkyboxNames{
	{
		{"maps/c0a0.bsp", "desert"},
		{"maps/c0a0a.bsp", "desert"},
		{"maps/c0a0b.bsp", "2desert"},
		{"maps/c0a0c.bsp", "desert"},
		{"maps/c0a0d.bsp", "desert"},
		{"maps/c0a0e.bsp", "desert"},
		{"maps/c1a0.bsp", "desert"},
		{"maps/c1a0a.bsp", "desert"},
		{"maps/c1a0b.bsp", "desert"},
		{"maps/c1a0c.bsp", "desert"},
		{"maps/c1a0d.bsp", "desert"},
		{"maps/c1a0e.bsp", "xen9"},
		{"maps/c1a1.bsp", "desert"},
		{"maps/c1a1a.bsp", "desert"},
		{"maps/c1a1b.bsp", "desert"},
		{"maps/c1a1c.bsp", "desert"},
		{"maps/c1a1d.bsp", "desert"},
		{"maps/c1a1f.bsp", "desert"},
		{"maps/c1a2.bsp", "desert"},
		{"maps/c1a2a.bsp", "desert"},
		{"maps/c1a2b.bsp", "desert"},
		{"maps/c1a2c.bsp", "desert"},
		{"maps/c1a2d.bsp", "desert"},
		{"maps/c1a3.bsp", "desert"},
		{"maps/c1a3a.bsp", "desert"},
		{"maps/c1a3b.bsp", "dusk"},
		{"maps/c1a3c.bsp", "dusk"},
		{"maps/c1a3d.bsp", "desert"},
		{"maps/c1a4.bsp", "desert"},
		{"maps/c1a4b.bsp", "desert"},
		{"maps/c1a4d.bsp", "desert"},
		{"maps/c1a4e.bsp", "desert"},
		{"maps/c1a4f.bsp", "desert"},
		{"maps/c1a4g.bsp", "desert"},
		{"maps/c1a4i.bsp", "desert"},
		{"maps/c1a4j.bsp", "desert"},
		{"maps/c1a4k.bsp", "desert"},
		{"maps/c2a1.bsp", "desert"},
		{"maps/c2a1a.bsp", "desert"},
		{"maps/c2a1b.bsp", "desert"},
		{"maps/c2a2.bsp", "night"},
		{"maps/c2a2a.bsp", "day"},
		{"maps/c2a2b1.bsp", "desert"},
		{"maps/c2a2b2.bsp", "desert"},
		{"maps/c2a2c.bsp", "desert"},
		{"maps/c2a2d.bsp", "desert"},
		{"maps/c2a2e.bsp", "night"},
		{"maps/c2a2f.bsp", "desert"},
		{"maps/c2a2g.bsp", "night"},
		{"maps/c2a2h.bsp", "night"},
		{"maps/c2a3.bsp", "desert"},
		{"maps/c2a3a.bsp", "desert"},
		{"maps/c2a3b.bsp", "desert"},
		{"maps/c2a3c.bsp", "dawn"},
		{"maps/c2a3d.bsp", "2desert"},
		{"maps/c2a3e.bsp", "desert"},
		{"maps/c2a4.bsp", "morning"},
		{"maps/c2a4a.bsp", "desert"},
		{"maps/c2a4b.bsp", "desert"},
		{"maps/c2a4c.bsp", "desert"},
		{"maps/c2a4d.bsp", "desert"},
		{"maps/c2a4e.bsp", "desert"},
		{"maps/c2a4f.bsp", "desert"},
		{"maps/c2a4g.bsp", "desert"},
		{"maps/c2a5.bsp", "desert"},
		{"maps/c2a5a.bsp", "cliff"},
		{"maps/c2a5b.bsp", "desert"},
		{"maps/c2a5c.bsp", "desert"},
		{"maps/c2a5d.bsp", "desert"},
		{"maps/c2a5e.bsp", "desert"},
		{"maps/c2a5f.bsp", "desert"},
		{"maps/c2a5g.bsp", "desert"},
		{"maps/c2a5w.bsp", "desert"},
		{"maps/c2a5x.bsp", "desert"},
		{"maps/c3a1.bsp", "desert"},
		{"maps/c3a1a.bsp", "desert"},
		{"maps/c3a1b.bsp", "desert"},
		{"maps/c3a2.bsp", "desert"},
		{"maps/c3a2a.bsp", "desert"},
		{"maps/c3a2b.bsp", "desert"},
		{"maps/c3a2c.bsp", "desert"},
		{"maps/c3a2d.bsp", "desert"},
		{"maps/c3a2e.bsp", "desert"},
		{"maps/c3a2f.bsp", "desert"},
		{"maps/c4a1.bsp", "neb7"},
		{"maps/c4a1a.bsp", "neb7"},
		{"maps/c4a1b.bsp", "alien1"},
		{"maps/c4a1c.bsp", "xen8"},
		{"maps/c4a1d.bsp", "xen8"},
		{"maps/c4a1e.bsp", "xen8"},
		{"maps/c4a1f.bsp", "black"},
		{"maps/c4a2.bsp", "neb6"},
		{"maps/c4a2a.bsp", "neb6"},
		{"maps/c4a2b.bsp", "neb6"},
		{"maps/c4a3.bsp", "black"},
		{"maps/c5a1.bsp", "xen10"}
	}
};
