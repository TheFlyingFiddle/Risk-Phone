/*
 * resource.cpp
 *
 *  Created on: 18 jul 2014
 *      Author: Lukas
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include "external_libs/lodepng.h"
#include "types.h"
#include "assert.h"
#include "resources.h"
#include "platform.h"
#include "path.h"
#include "remote_debug.h"

static void loadImage(const char* path, uint8_t** data, uint32_t* width, uint32_t* height)
{
	auto resource = platformLoadResource(path);
	RLOGI("%s", "Loading png.");
	auto err = lodepng_decode32(data, width, height, resource.buffer, resource.length);
	ASSERTF(err == 0, "Failed to load png! ERROR: %s BUF: %d, LEN: %d",
			lodepng_error_text(err), (uint32_t)resource.buffer, resource.length);
	RLOGI("%s", "LOADED PNG");
}

static void checkGlError(const char* op) {
	for (GLint error = glGetError(); error; error = glGetError()) {
		switch (error)
		{
		case GL_INVALID_VALUE:
			RLOGE("after %s glError invalid value occured", op);
			break;
		case GL_INVALID_ENUM:
			RLOGE("after %s glError invalid enum occured", op);
			break;
		case GL_INVALID_OPERATION:
			RLOGE("after %s glError invalid operation occured", op);
			break;
		}
	}
}

Texture loadTexture(const char* path)
{
	Profile profile("Loading texture ");

	uint8_t* data;
	uint32_t width, height;
	loadImage(path, &data, &width, &height);

	GLuint name;
	glGenTextures(1, &name);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, name);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	checkGlError("textureHasError");

	free(data);

	return (Texture) { name, width, height };
}

void unloadTexture(Texture tex)
{
	Profile profile("Unloading texture");
	glDeleteTextures(1, &tex.glName);
}

Texture reloadTexture(const char* path, Texture tex)
{
	Profile profile("Reloading texture");

	uint8_t* data;
	uint32_t width, height;
	loadImage(path, &data, &width, &height);

	glBindTexture(GL_TEXTURE_2D, tex.glName);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	tex.width = width;
	tex.height = height;

	return tex;
}


typedef struct
{
	float size, lineHeight;
	uint dataOffset, dataLength;
	uint layer, hashID;
} FontHeader;

static size_t loadHeader(Resource asset, FontHeader** fonts, size_t* length)
{
	Buffer buf = bufferWrapArray(asset.buffer, asset.length);
	uint16_t count = bufferReadShort(&buf);
	*length = count;
	*fonts = (FontHeader*)(asset.buffer);

	memmove(asset.buffer, asset.buffer + sizeof(uint16_t), count * sizeof(FontHeader));

	//For some strange reason this in nessesary or it will crash on reading floating point stuff.
	//FontHeader* header = (FontHeader*)malloc(count * sizeof(FontHeader));
	//memcpy(header, asset.buffer + sizeof(uint16_t), count * sizeof(FontHeader));
	//*fonts = header;

	return sizeof(uint16_t) + count * sizeof(FontHeader);
}

#include "remote_debug.h"

static FontAtlas* loadFontAtlas(Resource fontAsset, Texture texture)
{
	FontHeader* header;
	size_t headerLength;
	size_t headerSize = loadHeader(fontAsset, &header, &headerLength);

	int dataSize = sizeof(FontAtlas) + sizeof(Font) * headerLength;
	int length = dataSize;
	for (int i = 0; i < headerLength; i++)
	{
		length += header[i].dataLength * sizeof(CharInfo);
	}
	auto data = (uint8_t*)malloc(length);
	memcpy(data + dataSize, fontAsset.buffer + headerSize, fontAsset.length - headerSize);

	auto atlas = (FontAtlas*)data;
	atlas->page 	   = texture;
	atlas->fontsLength = headerLength;
	atlas->fonts       = (Font*)(data + sizeof(FontAtlas));
	for(int i = 0; i < headerLength; i++)
	{
		auto font = &atlas->fonts[i];
		font->size = header[i].size;
		font->lineHeight = header[i].lineHeight;
		font->page	= texture;
		int start = dataSize + header[i].dataOffset;
		font->chars = (CharInfo*)((data) + start);
		font->charsLength = header[i].dataLength;
		font->layer  = header[i].layer;
		font->hashID = header[i].hashID;

		if(font->layer == 0)
			font->layer = 2;
		else if(font->layer == 2)
			font->layer = 0;

		font->defaultChar = '_';
	}

	return atlas;
}

FontAtlas* loadFont(const char* path)
{
	Profile profile("Loading font atlas");

	auto texture = loadTexture(path::changeExtension(path, ".png").c_str());
	Resource fontAsset = platformLoadResource(path);
	auto atlas = loadFontAtlas(fontAsset, texture);
	platformUnloadResource(fontAsset);
	return atlas;
}


void unloadFont(FontAtlas* font)
{
	Profile profile("Unloading font atlas");

	unloadTexture(font->page);
	free(font);
}

FontAtlas* reloadFont(const char* path, FontAtlas* atlas)
{
	Profile profile("Reloading font atlas");

	Texture texture = reloadTexture(path::changeExtension(path, ".png").c_str(), atlas->page);
	free(atlas);

	Resource fontAsset = platformLoadResource(path);
	auto newAtlas = loadFontAtlas(fontAsset, texture);
	platformUnloadResource(fontAsset);
	return newAtlas;
}
