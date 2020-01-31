#include "TabGui.h"

struct SelectedItemInformation {
	int selectedItemId = 0;
	float currentSelectedItemInterpol = 0;

	void setSelectedItemForce(int item) {
		selectedItemId = item;
		currentSelectedItemInterpol = (float)item;
	}

	void interp() {
		currentSelectedItemInterpol += (selectedItemId - currentSelectedItemInterpol) * 0.1f;
	}
};

// State
int level;
SelectedItemInformation selected[4];
bool toggleCurrentSelection = false;

// Render
static float yOffset;
static float xOffset;
int renderedLevel;

struct LabelContainer {
	const char* text = 0;
	bool enabled = false;
	IModule* mod = 0;
};

std::vector<LabelContainer> labelList;

void TabGui::renderLabel(const char* text, IModule* mod) {
	//size_t strlength = strlen(text) + 1;
	//char* alloc = new char[strlength];
	//strcpy_s(alloc, strlength, text);
	LabelContainer yikes;
	yikes.text = text;
	if (mod != 0) {
		yikes.enabled = mod->isEnabled();
		yikes.mod = mod;
	}

	labelList.push_back(yikes);
}

void TabGui::renderLevel() {
	// Parameters
	static constexpr float textSize = 1.f;
	static const float textHeight = 10.f * textSize;
	static constexpr float alphaVal = 1.0f;

	// First loop: Get the maximum text length
	float maxLength = 1;
	int labelListLength = 0;
	for (auto it = labelList.begin(); it != labelList.end(); ++it) {
		labelListLength++;
		std::string label = it->text;
		maxLength = max(maxLength, DrawUtils::getTextWidth(&label, textSize));
	}

	if (selected[renderedLevel].selectedItemId < 0)
		selected[renderedLevel].selectedItemId += labelListLength;
	if (selected[renderedLevel].selectedItemId >= labelListLength)
		selected[renderedLevel].selectedItemId -= labelListLength;

	selected[renderedLevel].interp();  // Converge to selected item

	// Second loop: Render everything
	int i = 0;
	float selectedYOffset = yOffset;
	float startYOffset = yOffset;
	for (auto it = labelList.begin(); it != labelList.end(); ++it, i++) {
		auto label = *it;
		vec4_t rectPos = vec4_t(
			xOffset - 0.5f,  // Off screen / Left border not visible
			yOffset,
			xOffset + maxLength + 4.5f,
			yOffset + textHeight);

		MC_Color color = MC_Color(200, 200, 200, 1);

		if (selected[renderedLevel].selectedItemId == i && level >= renderedLevel) {  // We are selected
			if (renderedLevel == level) {                                             // Are we actually in the menu we are drawing right now?
				// We are selected in the current menu
				DrawUtils::fillRectangle(rectPos, MC_Color(13, 29, 48, 1), 1.f);
				static bool lastVal = toggleCurrentSelection;

				if (toggleCurrentSelection) {
					if (label.mod->isFlashMode()) {
						label.mod->setEnabled(true);
					} else {
						toggleCurrentSelection = false;
						label.mod->toggle();
					}
				} else if (toggleCurrentSelection != lastVal && label.mod->isFlashMode())
					label.mod->setEnabled(false);
				lastVal = toggleCurrentSelection;
			} else {  // selected, but not what the user is interacting with
				DrawUtils::fillRectangle(rectPos, MC_Color(13, 29, 48, 1), 1.f);
			}
			selectedYOffset = yOffset;
		} else {  // We are not selected
			DrawUtils::fillRectangle(rectPos, MC_Color(13, 29, 48, 1), 1.f);
		}

		std::string tempLabel(label.text);
		DrawUtils::drawText(vec2_t(xOffset + 1.5f, yOffset /*+ 0.5f*/), &tempLabel, label.enabled ? MC_Color() : color, textSize);

		yOffset += textHeight;
	}

	// Draw selected item
	{
		float selectedOffset = startYOffset + textHeight * selected[renderedLevel].currentSelectedItemInterpol;
		vec4_t selectedPos = vec4_t(
			xOffset - 0.5f,  // Off screen / Left border not visible
			selectedOffset,
			xOffset + maxLength + 4.5f,
			selectedOffset + textHeight);

		if (renderedLevel <= level)
			DrawUtils::fillRectangle(selectedPos, MC_Color(28, 107, 201, 1), alphaVal);
	}

	// Cleanup
	DrawUtils::flush();
	labelList.clear();
	xOffset += maxLength + 4.5f;
	yOffset = selectedYOffset;
	renderedLevel++;
}

void TabGui::render() {
	if (!moduleMgr->isInitialized())
		return;
	renderedLevel = 0;
	yOffset = 4;
	xOffset = 3;

	// Render all categorys
	renderLabel("Combat");
	renderLabel("Visual");
	renderLabel("Movement");
	renderLabel("Player");
	renderLabel("Build");
	renderLabel("Exploits");
	renderLevel();

	// Render all modules
	if (level >= 0) {
		std::vector<IModule*>* modules = moduleMgr->getModuleList();
		for (std::vector<IModule*>::iterator it = modules->begin(); it != modules->end(); ++it) {
			IModule* mod = *it;
			if (selected[0].selectedItemId == static_cast<int>(mod->getCategory())) {
				auto name = mod->getModuleName();
				renderLabel(name, mod);
			}
		}
		renderLevel();
	}
}

void TabGui::init() {
	level = 0;
	xOffset = 0;
	yOffset = 0;
	renderedLevel = 0;
}

void TabGui::onKeyUpdate(int key, bool isDown) {
	if (!isDown) {
		if (key == VK_RIGHT)
			toggleCurrentSelection = false;
		return;
	}

	switch (key) {
	case VK_LEFT:  // Leave menus
		if (level > -1) {
			level--;
		}
		return;
	case VK_RIGHT:
		if (level < 1) {
			level++;
			selected[level].setSelectedItemForce(0);
		} else
			toggleCurrentSelection = true;
		return;
	case VK_UP:
		if (level >= 0)
			selected[level].selectedItemId--;
		else
			level = 0;
		break;
	case VK_DOWN:
		if (level >= 0)
			selected[level].selectedItemId++;
		else
			level = 0;
		break;
	};

	if (level < 3)
		selected[level + 1].setSelectedItemForce(0);
}
