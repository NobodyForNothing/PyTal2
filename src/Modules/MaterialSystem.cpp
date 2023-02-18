#include "MaterialSystem.hpp"

#include "Cheats.hpp"
#include "Command.hpp"
#include "Engine.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Offsets.hpp"
#include "PytalMain.hpp"
#include "Utils.hpp"
#include "Features/OverlayRender.hpp"

class MemTexRegen : public ITextureRegenerator {
	uint8_t *bgra;
	int w, h;

public:
	MemTexRegen(uint8_t *bgra, int w, int h) : w(w), h(h) {
		this->bgra = new uint8_t[w * h * 4];
		memcpy(this->bgra, bgra, w * h * 4);
	}

	~MemTexRegen() {
		delete[] this->bgra;
	}

	virtual void RegenerateTextureBits(ITexture *tex, IVTFTexture *vtf_tex, Rect_t *rect) override {
		auto ImageFormat_fn = Memory::VMT<ImageFormat (__rescall *)(IVTFTexture *)>(vtf_tex, Offsets::ImageFormat);
		ImageFormat fmt = ImageFormat_fn(vtf_tex);

		if (fmt != IMAGE_FORMAT_BGRA8888) {
			console->Print("ERROR: MemTexRegen got unexpected image format %d\n", (int)fmt);
			return;
		}

		auto ImageData = Memory::VMT<uint8_t *(__rescall *)(IVTFTexture *)>(vtf_tex, Offsets::ImageData);
		memcpy(ImageData(vtf_tex), this->bgra, this->w * this->h * 4);
	}

	virtual void Release() override {
		delete this;
	}
};

REDECL(MaterialSystem::UncacheUnusedMaterials);
REDECL(MaterialSystem::CreateMaterial);

IMaterial *MaterialSystem::FindMaterial(const char *materialName, const char *textureGroupName) {
	auto func = (IMaterial *(__rescall *)(void *, const char *, const char *, bool, const char *))this->materials->Current(Offsets::FindMaterial);
	return func(this->materials->ThisPtr(), materialName, textureGroupName, true, nullptr);
}
ITexture *MaterialSystem::CreateTexture(const char *name, int w, int h, uint8_t *bgra) {
	auto CreateProceduralTexture = (ITexture *(__rescall *)(void *, const char *, const char *, int, int, ImageFormat, int))this->materials->Current(Offsets::CreateProceduralTexture);
	// For any 8-bit alpha raw format, the regeneration will use BGRA8888 in the
	// VTF texture, so we may as well mirror that in the actual texture too. Note
	// that this also means the argument to this function must be BGRA unless we
	// implement some custom conversion logic in MemTexRegen
	ITexture *tex = CreateProceduralTexture(this->materials->ThisPtr(), name, "PYTAL textures", w, h, IMAGE_FORMAT_BGRA8888, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_SINGLECOPY);
	if (!tex) return nullptr;

	tex->SetTextureRegenerator(new MemTexRegen(bgra, w, h));
	tex->Download();

	return tex;

	// TODO: destroy textures
}
IMatRenderContext *MaterialSystem::GetRenderContext() {
	auto func = (IMatRenderContext *(__rescall *)(void *))this->materials->Current(Offsets::GetRenderContext);
	return func(this->materials->ThisPtr());
}

MaterialSystem *materialSystem;
