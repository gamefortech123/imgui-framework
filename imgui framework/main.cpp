#include "src/core/graphics_api.hpp"
#include "src/core/config.hpp"

#include "src/widgets/widgets.hpp"
#include "src/widgets/fonts.hpp"
#include "src/widgets/crosshairs.hpp"

// internal use
bool g_open = true;
bool g_toggledInput = false;

// config variables
bool& g_bCrosshair = Config::Add("crosshair", "General", Config::Type::BOOL, true);
bool& g_bInfoOverlay = Config::Add("info_overlay", "General", Config::Type::BOOL, true);
int& g_menuKeyBind = Config::Add("menu", "Key Binds", Config::Type::INT, VK_INSERT);
ImVec2& g_crosshairSize = Config::Add("crosshair", "Sizes", Config::Type::IMVEC2, ImVec2(75.f, 75.f));

ImFont* g_pLargeFont = nullptr;

struct ImageTex {
	std::string m_name;
	PDIRECT3DTEXTURE9 m_texture;
	int m_width;
	int m_height;
};

std::vector<ImageTex> g_crosshairs;
ImageTex* g_pActiveCrosshair = nullptr;

bool WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return true;
}

void InitUI() {
	ImGuiStyle& s = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();

	// load fonts
	io.Fonts->AddFontDefault();
	constexpr int largeFontSize = 24;
	g_pLargeFont = io.Fonts->AddFontFromMemoryTTF((void*)g_rudaBold, largeFontSize, largeFontSize);

	// load crosshairs
	// load default crosshairs
	ImageTex defaultTex = {};
	defaultTex.m_name = "default csgo crosshair";
	if (GAPI::LoadTextureFromMemory(g_defaultCrosshair.data(), g_defaultCrosshair.size(), &defaultTex.m_texture, &defaultTex.m_width, &defaultTex.m_height))
		g_crosshairs.push_back(defaultTex);

	auto files = Utils::GetFilesInDirectory("d://crosshairs");
	for (auto& file : files) {
		if (!Utils::EndsWith(file, ".png"))
			continue;

		ImageTex imageTex = {};
		imageTex.m_name = file;
		if (GAPI::LoadTextureFromFile(imageTex.m_name.c_str(), &imageTex.m_texture, &imageTex.m_width, &imageTex.m_height))
			g_crosshairs.push_back(imageTex);
	}

	// set styles
	s.WindowRounding = 0.f;
}

void RenderUI() {
	GAPI::ZoomRegion({ 100, 100 }, { 200, 200 }, 2.f);

	if (g_open) {
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoSavedSettings;
		flags |= ImGuiWindowFlags_NoCollapse;
		flags |= ImGuiWindowFlags_NoNav;

		ImGui::Begin("test", (bool*)1, flags); {
			if (ImGui::Button("click me")) {
				printf("works\n");
			}

			ImGui::Hotkey("menu keybind", &g_menuKeyBind);

			ImGui::Checkbox("crosshair", &g_bCrosshair);
			ImGui::Checkbox("info overlay", &g_bInfoOverlay);

			if (ImGui::ListBoxHeader("crosshairs")) {
				for (auto& crosshair : g_crosshairs) {
					bool selected = (g_pActiveCrosshair == &crosshair);
					if (ImGui::Selectable(crosshair.m_name.c_str(), selected)) {
						g_pActiveCrosshair = &crosshair;
					}
				}

				ImGui::ListBoxFooter();
			}

			if (g_pActiveCrosshair) {
				ImGui::SliderFloat("crosshair width", &g_crosshairSize.x, 1, 200);
				ImGui::SliderFloat("crosshair height", &g_crosshairSize.y, 1, 200);
			}

			if (ImGui::Button("Save Settings")) {
				Config::Save();
			}

		} ImGui::End();
	}

	// render other windows here that should be displayed regardless of menu being open or not
	POINT mPos;
	GetCursorPos(&mPos);

	ImGui::Begin("info window"); {

		if (!g_open) {
			auto pos = ImGui::GetWindowPos();
			auto size = ImGui::GetWindowSize();

			bool isHovering = mPos.x >= pos.x && mPos.y >= pos.y && mPos.x <= (pos.x + size.x) && mPos.y <= (pos.y + size.y);

			if (!g_toggledInput && isHovering) {
				GAPI::EnableInput();
				printf("enabled\n");
				g_toggledInput = true;
			}

			else if (g_toggledInput && !isHovering) {
				GAPI::DisableInput();
				printf("disabled\n");
				g_toggledInput = false;
			}

			ImGui::Text("pos:  { %f, %f }", pos.x, pos.y);
			ImGui::Text("size: { %f, %f }", size.x, size.y);
			ImGui::Text("mpos: { %i, %i }", mPos.x, mPos.y);
			ImGui::Text("is hovering: %i", isHovering);
		}
	} ImGui::End();

	// render background overlay widgets
	auto* pBgDrawList = ImGui::GetBackgroundDrawList();
	ImGui::PushFont(g_pLargeFont);

#pragma region overlay_info
	if (g_bInfoOverlay) {
		std::vector<std::pair<std::string, ImColor>> overlayInfoText = {
			{"mouse positions: " + std::to_string(mPos.x) + ", " + std::to_string(mPos.y), ImColor(255, 0, 0)},
			{"This is an overlay for information", ImColor(0, 255, 0)},
			{"and more stuff...", ImColor(0, 0, 255)}
		};

		ImVec2 maxSize = { 0, 0 };
		for (auto& infoText : overlayInfoText) {
			auto size = ImGui::CalcTextSize(infoText.first.c_str());
			if (size.x >= maxSize.x)
				maxSize.x = size.x;
			if (size.y >= maxSize.y)
				maxSize.y = size.y;
		}

		constexpr int padding = 10;
		int startPosX = GAPI::GetWindowSize().x - (maxSize.x + (padding * 2));

		ImVec2 bgStart = { (float)startPosX - padding, padding * 0.5f };
		ImVec2 bgEnd = { bgStart.x + maxSize.x + (padding * 2), bgStart.y + (overlayInfoText.size() * maxSize.y) + padding };
		pBgDrawList->AddRectFilled(bgStart, bgEnd, ImColor(11, 11, 11, 225));

		for (int i = 0; i < overlayInfoText.size(); i++) {
			pBgDrawList->AddText({ (float)startPosX, padding + (i * maxSize.y) }, overlayInfoText[i].second, overlayInfoText[i].first.c_str());
		}
	}
#pragma endregion

	if (g_bCrosshair && g_pActiveCrosshair) {
		auto windowSize = GAPI::GetWindowSize();
		ImVec2 start = { (windowSize.x - g_crosshairSize.x) / 2.f, (windowSize.y - g_crosshairSize.y) / 2.f };
		ImVec2 end = { start.x + g_crosshairSize.x, start.y + g_crosshairSize.y };
		pBgDrawList->AddImage(g_pActiveCrosshair->m_texture, start, end);
	}

	ImGui::PopFont();
}

auto main() -> int
{
	GAPI::MakeWindow(
		L"hook_with_imgui", // window title
		L"c_hook_with_imgui", // window class
		WndProc,
		InitUI,
		RenderUI,
		&g_open,
		&g_menuKeyBind
	);

	return TRUE;
}
