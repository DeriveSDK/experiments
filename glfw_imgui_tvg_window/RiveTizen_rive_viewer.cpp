#include "TvgWindow.h"

#include "rive/animation/linear_animation_instance.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_trigger.hpp"
#include "rive/artboard.hpp"
#include "rive/file.hpp"
#include "rive/layout.hpp"
#include "rive/math/aabb.hpp"
#include "tvg_renderer.hpp"

#include <stdio.h>

// ImGui wants raw pointers to names, but our public API returns
// names as strings (by value), so we cache these names each time we
// load a file
std::vector<std::string> animationNames;
std::vector<std::string> stateMachineNames;

class RiveExample : public TvgWindow {
protected:
	std::string filename;
	std::unique_ptr<rive::File> currentFile;
	std::unique_ptr<rive::ArtboardInstance> artboardInstance;
	std::unique_ptr<rive::StateMachineInstance> stateMachineInstance;
	std::unique_ptr<rive::LinearAnimationInstance> animationInstance;

	// We hold onto the file's bytes for the lifetime of the file, in case we want
	// to change animations or state-machines, we just rebuild the rive::File from
	// it.
	std::vector<uint8_t> fileBytes;

	int animationIndex = 0;
	int stateMachineIndex = -1;

	std::unique_ptr<rive::TvgRenderer> renderer = nullptr;
public:
	/**
	 * Pass-through constructor
	 */
	RiveExample() : TvgWindow() {}

	/**
	 * Set up renderer. Start listening for dropped files.
	 */
	void setup() override {
		renderer = std::unique_ptr<rive::TvgRenderer>(new rive::TvgRenderer(canvas.get()));
		clearColor = 0xffff9900;
	}
	/**
	 * Draw thorvg elements
	 **/
	void update(double dt) override {
		if (artboardInstance != nullptr) {
			if (animationInstance != nullptr) {
				animationInstance->advance(dt);
				animationInstance->apply();
			}
			else if (stateMachineInstance != nullptr) {
				stateMachineInstance->advance(dt);
			}
			artboardInstance->advance(dt);

			// Render the rive animation
			canvas->clear();
			renderer->save();
			renderer->align(rive::Fit::contain,
				rive::Alignment::center,
				rive::AABB(0, 0, width, height),
				artboardInstance->bounds());
			applyMouseEvent(artboardInstance.get());
			artboardInstance->draw(renderer.get());
			renderer->restore();
			if (canvas->draw() == tvg::Result::Success) canvas->sync();
		}
	}
	void updateGui(double dt) override {
		if (artboardInstance != nullptr) {
			
			ImGui::Begin(filename.c_str(), nullptr);
			if (animationNames.size() > 0) {
				ImGui::Text("Animations");
				if (ImGui::ListBox(
					"##Animations",
					&animationIndex,
					[](void* data, int index, const char** name) {
						*name = animationNames[index].c_str();
						return true;
					},
					artboardInstance.get(),
						animationNames.size(),
						4))
				{
					stateMachineIndex = -1;
					initAnimation(animationIndex);
				}
			}
			if (stateMachineNames.size() > 0) {
				ImGui::Text("State Machines");
				if (ImGui::ListBox(
					"##State Machines",
					&stateMachineIndex,
					[](void* data, int index, const char** name) {
						*name = stateMachineNames[index].c_str();
						return true;
					},
					artboardInstance.get(),
						stateMachineNames.size(),
						4))
				{
					animationIndex = -1;
					initStateMachine(stateMachineIndex);
				}
			}
			if (stateMachineInstance != nullptr) {

				ImGui::Columns(2);
				ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.6666);

				for (int i = 0; i < stateMachineInstance->inputCount(); i++) {
					auto inputInstance = stateMachineInstance->input(i);

					if (inputInstance->input()->is<rive::StateMachineNumber>()) {
						// ImGui requires names as id's, use ## to hide the
						// label but still give it an id.
						char label[256];
						snprintf(label, 256, "##%u", i);

						auto number = static_cast<rive::SMINumber*>(inputInstance);
						float v = number->value();
						ImGui::InputFloat(label, &v, 1.0f, 2.0f, "%.3f");
						number->value(v);
						ImGui::NextColumn();
					}
					else if (inputInstance->input()->is<rive::StateMachineTrigger>()) {
						// ImGui requires names as id's, use ## to hide the
						// label but still give it an id.
						char label[256];
						snprintf(label, 256, "Fire##%u", i);
						if (ImGui::Button(label)) {
							auto trigger = static_cast<rive::SMITrigger*>(inputInstance);
							trigger->fire();
						}
						ImGui::NextColumn();
					}
					else if (inputInstance->input()->is<rive::StateMachineBool>()) {
						// ImGui requires names as id's, use ## to hide the
						// label but still give it an id.
						char label[256];
						snprintf(label, 256, "##%u", i);
						auto boolInput = static_cast<rive::SMIBool*>(inputInstance);
						bool value = boolInput->value();

						ImGui::Checkbox(label, &value);
						boolInput->value(value);
						ImGui::NextColumn();
					}
					ImGui::Text("%s", inputInstance->input()->name().c_str());
					ImGui::NextColumn();
				}

				ImGui::Columns(1);
			}
			ImGui::Text("FPS %d", int(fps));
			ImGui::End();
		}
		else {
			ImGui::Text("Drop a .riv file to preview.");
		}

	}
	void cleanup() override {
		
	}

	/**
	 * Load a rive file that has been dropped on the window.
	 * If multiple files, just take the last one
	 */
	void onFilesDropped(std::vector<std::string> paths) override{
		filename = paths[paths.size() - 1];
		FILE* fp = fopen(filename.c_str(), "rb");
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fileBytes.resize(size);
		if (fread(fileBytes.data(), 1, size, fp) != size) {
			fileBytes.clear();
			return;
		}
		initAnimation(0);
	}

	void loadNames(const rive::Artboard* ab) {
		animationNames.clear();
		stateMachineNames.clear();
		if (ab) {
			for (size_t i = 0; i < ab->animationCount(); ++i) {
				animationNames.push_back(ab->animationNameAt(i));
			}
			for (size_t i = 0; i < ab->stateMachineCount(); ++i) {
				stateMachineNames.push_back(ab->stateMachineNameAt(i));
			}
		}
	}

	void initStateMachine(int index) {
		stateMachineIndex = index;
		animationIndex = -1;
		assert(fileBytes.size() != 0);
		rive::BinaryReader reader(rive::toSpan(fileBytes));
		auto file = rive::File::import(reader);
		if (!file) {
			fileBytes.clear();
			fprintf(stderr, "failed to import file\n");
			return;
		}
		animationInstance = nullptr;
		stateMachineInstance = nullptr;
		artboardInstance = nullptr;

		currentFile = std::move(file);
		artboardInstance = currentFile->artboardDefault();
		artboardInstance->advance(0.0f);
		loadNames(artboardInstance.get());

		if (index >= 0 && index < artboardInstance->stateMachineCount()) {
			stateMachineInstance = artboardInstance->stateMachineAt(index);
		}
	}

	void initAnimation(int index) {
		animationIndex = index;
		stateMachineIndex = -1;
		assert(fileBytes.size() != 0);
		rive::BinaryReader reader(rive::toSpan(fileBytes));
		auto file = rive::File::import(reader);
		if (!file) {
			fileBytes.clear();
			fprintf(stderr, "failed to import file\n");
			return;
		}
		animationInstance = nullptr;
		stateMachineInstance = nullptr;
		artboardInstance = nullptr;

		currentFile = std::move(file);
		artboardInstance = currentFile->artboardDefault();
		artboardInstance->advance(0.0f);
		loadNames(artboardInstance.get());

		if (index >= 0 && index < artboardInstance->animationCount()) {
			animationInstance = artboardInstance->animationAt(index);
		}
	}

	void applyMouseEvent(rive::Artboard* artboard) {
		static ImVec2 gPrevMousePos = { -1000, -1000 };
		const auto mouse = ImGui::GetMousePos();

		static bool gPrevMouseButtonDown = false;
		const bool isDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

		if (mouse.x == gPrevMousePos.x &&
			mouse.y == gPrevMousePos.y &&
			isDown == gPrevMouseButtonDown)
		{
			return;
		}

		auto evtType = rive::PointerEventType::move;
		if (isDown && !gPrevMouseButtonDown) {
			evtType = rive::PointerEventType::down; // we just went down
		}
		else if (!isDown && gPrevMouseButtonDown) {
			evtType = rive::PointerEventType::up; // we just went up
		}

		gPrevMousePos = mouse;
		gPrevMouseButtonDown = isDown;

		const int pointerIndex = 0; // til we track more than one button/mouse
		rive::PointerEvent evt = {
			evtType,
			{mouse.x, mouse.y},
			pointerIndex,
		};
		artboard->postPointerEvent(evt);
	}
};

int main(void)
{
	RiveExample example;
	example.run();
}
