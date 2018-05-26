#include <rpg/editor.hpp>

using namespace editors;

#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>
#include <cstring>

#include <rpg/rpg_config.hpp>

#include <engine/logger.hpp>
#include <engine/utility.hpp>
#include <engine/math.hpp>

#include <imgui.h>
#include <imgui-SFML.h>

#include <editor/file_opener.hpp>
#include <editor/imgui_ext.hpp>

inline engine::fvector read_args_vector(const engine::terminal_arglist& pArgs, float pDefx = 0, float pDefy = 0, size_t pIndex = 0)
{
	engine::fvector ret;
	if (pArgs[pIndex].get_raw() == "-")
		ret.x = pDefx;
	else
		ret.x = util::to_numeral<float>(pArgs[pIndex]);

	if (pArgs[pIndex + 1].get_raw() == "-")
		ret.y = pDefy;
	else
		ret.y = util::to_numeral<float>(pArgs[pIndex + 1]);
	return ret;
}

void command_manager::cancel()
{
	mCurrent.reset();
}

void command_manager::complete()
{
	assert(mCurrent);
	mRedo.clear();
	mUndo.push_back(mCurrent);
	mCurrent.reset();
}

bool command_manager::execute_and_complete()
{
	mRedo.clear();
	mUndo.push_back(mCurrent);
	bool r = mCurrent->execute();
	mCurrent.reset();
	return r;
}

bool command_manager::undo()
{
	assert(!mCurrent);
	if (mUndo.size() == 0)
		return false;
	auto undo_cmd = mUndo.back();
	mUndo.pop_back();
	mRedo.push_back(undo_cmd);
	return undo_cmd->undo();
}

bool command_manager::redo()
{
	assert(!mCurrent);
	if (mRedo.size() == 0)
		return false;
	auto redo_cmd = mRedo.back();
	mRedo.pop_back();
	mUndo.push_back(redo_cmd);
	return redo_cmd->redo();
}

void command_manager::clear()
{
	mUndo.clear();
	mRedo.clear();
	mCurrent.reset();
}

class command_set_tiles :
	public command
{
public:
	command_set_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
	}

	command_set_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator, rpg::tile& pTile)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
		add(pTile);
	}

	bool execute()
	{
		assert(mTilemap_manipulator);

		// Track all the tiles that are replaced
		for (auto& i : mTiles)
		{
			auto replaced_tile = mTilemap_manipulator->get_layer(mLayer).find_tile(i.get_position());
			if (replaced_tile)
				mReplaced_tiles.push_back(*replaced_tile);
		}

		// Replace all the tiles
		for (auto& i : mTiles)
			mTilemap_manipulator->get_layer(mLayer).set_tile(i);

		return true;
	}

	bool undo()
	{
		assert(mTilemap_manipulator);

		// Remove all placed tiles
		for (auto& i : mTiles)
			mTilemap_manipulator->get_layer(mLayer).remove_tile(i.get_position());

		// Place all replaced tiles back
		for (auto& i : mReplaced_tiles)
			mTilemap_manipulator->get_layer(mLayer).set_tile(i);

		return true;
	}

	void add(rpg::tile& pTile)
	{
		mTiles.push_back(pTile);
	}

private:
	std::size_t mLayer;

	std::vector<rpg::tile> mReplaced_tiles;
	std::vector<rpg::tile> mTiles;

	rpg::tilemap_manipulator* mTilemap_manipulator;
};

class command_remove_tiles :
	public command
{
public:
	command_remove_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
	}

	command_remove_tiles(std::size_t pLayer, rpg::tilemap_manipulator& pTilemap_manipulator, engine::fvector pTile)
	{
		mLayer = pLayer;
		mTilemap_manipulator = &pTilemap_manipulator;
		add(pTile);
	}
	bool execute()
	{
		assert(mTilemap_manipulator);

		// Track all the tiles that are replaced
		for (auto& i : mTiles_to_remove)
		{
			auto replaced_tile = mTilemap_manipulator->get_layer(mLayer).find_tile(i);
			if (replaced_tile)
			{
				mRemoved_tiles.push_back(*replaced_tile);
				mTilemap_manipulator->get_layer(mLayer).remove_tile(i);
			}
		}

		return true;
	}

	bool undo()
	{
		for (auto& i : mRemoved_tiles)
			mTilemap_manipulator->get_layer(mLayer).set_tile(i);
		return true;
	}

	void add(engine::fvector pPosition)
	{
		mTiles_to_remove.push_back(pPosition);
	}

private:
	std::size_t mLayer;

	std::vector<rpg::tile> mRemoved_tiles;
	std::vector<engine::fvector> mTiles_to_remove;

	rpg::tilemap_manipulator* mTilemap_manipulator;
};

class command_add_layer :
	public command
{
public:
	command_add_layer(rpg::tilemap_manipulator& pTm_man, size_t pIndex)
	{
		mTilemap_manipulator = &pTm_man;
		mIndex = pIndex;
	}

	bool execute()
	{
		mTilemap_manipulator->insert_layer(mIndex);
		return true;
	}

	bool undo()
	{
		mLayer = std::move(mTilemap_manipulator->get_layer(mIndex));
		mTilemap_manipulator->remove_layer(mIndex);
		return true;
	}

	bool redo()
	{
		mTilemap_manipulator->insert_layer(mIndex);
		mTilemap_manipulator->get_layer(mIndex) = std::move(mLayer);
		return true;
	}

private:
	size_t mIndex;
	rpg::tilemap_manipulator* mTilemap_manipulator;
	rpg::tilemap_layer mLayer;
};

class command_delete_layer :
	public command
{
public:
	command_delete_layer(rpg::tilemap_manipulator& pTm_man, size_t pIndex)
	{
		mTilemap_manipulator = &pTm_man;
		mIndex = pIndex;
	}

	bool execute()
	{
		mLayer = std::move(mTilemap_manipulator->get_layer(mIndex));
		mTilemap_manipulator->remove_layer(mIndex);
		return true;
	}

	bool undo()
	{
		mTilemap_manipulator->insert_layer(mIndex);
		mTilemap_manipulator->get_layer(mIndex) = std::move(mLayer);
		return true;
	}

private:
	size_t mIndex;
	rpg::tilemap_manipulator* mTilemap_manipulator;
	rpg::tilemap_layer mLayer;
};

class command_move_layer :
	public command
{
public:
	command_move_layer(rpg::tilemap_manipulator& pTm_man, size_t pFrom, size_t pTo)
	{
		mTilemap_manipulator = &pTm_man;
		mFrom = pFrom;
		mTo = pTo;
	}

	bool execute()
	{
		 mTilemap_manipulator->move_layer(mFrom, mTo);
		 return true;
	}

	bool undo()
	{
		mTilemap_manipulator->move_layer(mTo, mFrom);
		return true;
	}

private:
	size_t mFrom, mTo;
	rpg::tilemap_manipulator* mTilemap_manipulator;
};

// ##########
// collisionbox_editor
// ##########

class command_add_wall :
	public command
{
public:
	command_add_wall(rpg::collision_box_container& pContainer, std::shared_ptr<rpg::collision_box> pBox)
	{
		mBox = pBox;
		mContainer = &pContainer;
	}

	bool execute()
	{
		mContainer->add_collision_box(mBox);
		return true;
	}

	bool undo()
	{
		mContainer->remove_box(mBox);
		return true;
	}

	bool redo()
	{
		return execute();
	}

private:
	std::shared_ptr<rpg::collision_box> mBox;
	rpg::collision_box_container* mContainer;
};

class command_remove_wall :
	public command
{
public:
	command_remove_wall(rpg::collision_box_container& pContainer, std::shared_ptr<rpg::collision_box> pBox)
		: pOpposing(pContainer, pBox)
	{}

	bool execute()
	{
		return pOpposing.undo();
	}

	bool undo()
	{
		return pOpposing.execute();
	}

	bool redo()
	{
		return execute();
	}

private:
	command_add_wall pOpposing;
};

class command_wall_changed :
	public command
{
public:
	command_wall_changed(std::shared_ptr<rpg::collision_box> pBox)
		: mBox(pBox), mOpposing(pBox->copy())
	{}

	bool execute()
	{
		std::shared_ptr<rpg::collision_box> temp(mBox->copy());
		mBox->set(mOpposing);
		mOpposing->set(temp);
		return true;
	}

	bool undo()
	{
		return execute();
	}

	bool redo()
	{
		return execute();
	}

private:
	std::shared_ptr<rpg::collision_box> mBox;
	std::shared_ptr<rpg::collision_box> mOpposing;
};

bool editor_settings_loader::load(const engine::fs::path& pPath)
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(pPath.string().c_str()) != tinyxml2::XML_SUCCESS)
		return false;
	auto ele_root = doc.RootElement();
	mPath = util::safe_string(ele_root->FirstChildElement("path")->GetText());
	mOpen_param = util::safe_string(ele_root->FirstChildElement("open")->GetText());
	mOpento_param = util::safe_string(ele_root->FirstChildElement("opento")->GetText());
	return true;
}

std::string editor_settings_loader::generate_open_cmd(const std::string & pFilepath) const
{
	std::string modified_param = mOpen_param;
	util::replace_all_with(modified_param, "%filename%", pFilepath);
	return mPath + " " + modified_param;
}

std::string editor_settings_loader::generate_opento_cmd(const std::string & pFilepath, size_t pRow, size_t pCol)
{
	std::string modified_param = mOpento_param;
	util::replace_all_with(modified_param, "%filename%", pFilepath);
	util::replace_all_with(modified_param, "%row%", std::to_string(pRow));
	util::replace_all_with(modified_param, "%col%", std::to_string(pCol));
	return mPath + " " + modified_param;
}

int OSA_distance(const std::string& pStr1, const std::string& pStr2)
{
	std::vector<std::vector<int>> d;
	d.resize(pStr1.length() + 1);
	for (std::size_t i = 0; i < d.size(); i++)
		d[i].resize(pStr2.length() + 1);

	for (std::size_t i = 0; i < pStr1.length() + 1; i++)
		d[i][0] = i;
	for (std::size_t i = 0; i < pStr2.length() + 1; i++)
		d[0][i] = i;

	for (std::size_t i = 1; i < pStr1.length() + 1; i++)
	{
		for (std::size_t j = 1; j < pStr2.length() + 1; j++)
		{
			std::size_t str1_i = i - 1;
			std::size_t str2_i = j - 1;
			int cost = (pStr1[str1_i] == pStr2[str2_i]) ? 0 : 1;
			d[i][j] = std::min({
					d[i - 1][j] + 1,
					d[i][j - 1] + 1,
					d[i - 1][j - 1] + cost
				});
			if (i > 1 && j > 1 && pStr1[str1_i] == pStr2[str2_i - 1] && pStr1[str1_i - 1] == pStr2[str2_i])
				d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + cost);
		}
	}
	return d[pStr1.length()][pStr2.length()];
}

static inline engine::fvector snap(const engine::fvector& pPos, const engine::fvector& pTo, const engine::fvector& pOffset = {0, 0})
{
	if (pTo.has_zero())
		return pPos;
	return (pPos / pTo - pOffset).floor() * pTo + pOffset;
}

static inline engine::fvector snap_closest(const engine::fvector& pPos, const engine::fvector& pTo, const engine::fvector& pOffset = { 0, 0 })
{
	if (pTo.has_zero())
		return pPos;
	return (pPos / pTo - pOffset).round() * pTo + pOffset;
}

static inline engine::frect snap_closest(const engine::frect& pRect, const engine::fvector& pTo, const engine::fvector& pOffset = { 0, 0 })
{
	return{
		snap_closest(pRect.get_offset(), pTo, pOffset),
		snap_closest(pRect.get_size(), pTo, pOffset)
	};
}



// Resizes a render texture if the imgui window was changed size.
// Works best if this is the first thing drawn in the window.
// Returns true if the texture was actually changed.
bool resize_to_window(sf::RenderTexture& pRender)
{
	sf::Vector2u window_size = static_cast<sf::Vector2u>(static_cast<sf::Vector2f>(ImGui::GetWindowContentRegionMax()))
		- sf::Vector2u(ImGui::GetCursorPos()) * (unsigned int)2;
	if (window_size != pRender.getSize())
	{
		pRender.create(window_size.x, window_size.y);
		return true;
	}
	return false;
}

WGE_imgui_editor::WGE_imgui_editor()
{
	mGame_render_target.create(400, 400);
	mGame_renderer.set_target_render(mGame_render_target);
	mGame.set_renderer(mGame_renderer);
	mAtlas_editor.set_resource_manager(mGame.get_resource_manager());
	mAtlas_editor.set_window_open_handler(&mWindow_open[window_open_atlas_editor]);

	mWindow_open.fill(true);

	mTile_size = 32;

	mSelected_tile = 1;
	mTile_rotation = 0;
	mTilemap_current_snapping = snapping_1x1;

	mSettings.load("./editor/settings.xml");

	mCurrent_layer = 0;
	mIs_scene_modified = false;
	mShow_debug_info = true;
	mShow_grid = true;
	mGrid_color = { 1, 1, 1, 0.7f };
	mCurrent_scene_editor = editor_tilemap;	

	mDebugger_selected_thread = 0;
	mDebugger_selected_stack = 0;
	
	ImGui::OpenRenderer(&mScene_editor_rendererdata);
	ImGui::UseRenderer(mScene_editor_rendererdata);
	ImGui::SetRendererUnit(static_cast<float>(mTile_size));
	mTilemap_display.set_parent(ImGui::GetRendererWorldNode());
	ImGui::EndRenderer();
}

WGE_imgui_editor::~WGE_imgui_editor()
{
	ImGui::CloseRenderer(&mScene_editor_rendererdata);
}

void WGE_imgui_editor::run()
{
	ImGui::LoadSettings("./editor/settings_ext.xml");

	{
		engine::fvector window_size(640, 480);
		ImGui::UpdateSetting("Window Size", &window_size);
		mWindow.initualize("WGE Editor New and improved", engine::vector_cast<int>(window_size));
	}

	ImGui::SFML::Init(mWindow.get_sfml_window());

	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::StyleColorsDark(&style);
	style.FrameRounding = 2;
	style.FramePadding = ImVec2(3, 2);

	if (!engine::fs::exists("./editor"))
		engine::fs::create_directories("./editor");
	ImGui::GetIO().IniFilename = "./editor/imgui.ini";
	ImGui::GetIO().LogFilename = "./editor/imgui_log.txt";

	mGame.load("./data");
	mGame_renderer.refresh();
	mScene_list = mGame.get_scene_list();

	mRunning = true;
	mIs_closing = false;

	sf::Clock delta_clock;
	while (mRunning)
	{
		if (!mWindow.poll_events())
			mIs_closing = true;
		mWindow.push_events_to_imgui();

		{
			ImGui::UpdateSetting("Window Size", &engine::vector_cast<float>(mWindow.get_size()));

			engine::fvector position = mWindow.get_position();
			if (ImGui::UpdateSetting("Window Position", &position))
				mWindow.set_position(position);

			for (std::size_t i = 0; i < mWindow_open.size(); i++)
				ImGui::UpdateSetting(std::string("Subwindow open " + std::to_string(i)).c_str(), &mWindow_open[i]);
			ImGui::UpdateSetting("Show Grid", &mShow_grid);
			ImGui::UpdateSetting("Grid Color", &mGrid_color);
			ImGui::UpdateSetting("Scene editor snapping", &mTilemap_current_snapping);
			ImGui::UpdateSetting("Show game debug info", &mShow_debug_info);
		}

		if (mIs_game_view_window_focused)
			mGame_renderer.update_events(mWindow);

		ImGui::SFML::Update(mWindow.get_sfml_window(), delta_clock.restart());

		mAtlas_editor.update();

		handle_save_confirmations();
		handle_undo_redo();

		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("View"))
		{
			const std::array<const char*, window_open_count> names =
			{
				"Game",
				"Scene Properties",
				"Scene Editor",
				"Atlas Editor",
				"Log",
				"Settings",
			};
			for (std::size_t i = 0; i < window_open_count; i++)
				if (ImGui::MenuItem(names[i], nullptr, mWindow_open[i]))
					mWindow_open[i] = !mWindow_open[i];
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		draw_scene_window();
		draw_game_window();
		draw_game_view_window();
		draw_scene_editor_window();
		draw_log_window();
		draw_settings_window();
		
		if (ImGui::Begin("Debugger"))
		{
			rpg::script_system& script = mGame.get_script_system();
			if (script.get_thread_count() > 0)
			{
				ImGui::BeginGroup();
				ImGui::TextUnformatted("Threads");
				ImGui::BeginChild("threadlist", ImVec2(300, 300), true);
				mDebugger_selected_thread = std::min(mDebugger_selected_thread, script.get_thread_count() - 1);
				for (size_t i = 0; i < script.get_thread_count(); i++)
				{
					if (ImGui::Selectable(script.get_thread_function(i)->GetDeclaration(true, true, true), mDebugger_selected_thread == i))
					{
						mDebugger_selected_thread = i;
						mDebugger_selected_stack = 0;
					}
				}
				ImGui::EndChild();
				ImGui::TextUnformatted("Call Stack");
				ImGui::BeginChild("callstack", ImVec2(300, 300), true);
				auto stack_info = script.get_stack_info(mDebugger_selected_thread);
				mDebugger_selected_stack = std::min(mDebugger_selected_stack, stack_info.size() - 1);
				for (size_t i = 0; i < stack_info.size(); i++)
				{
					if (ImGui::Selectable(stack_info[i].func->GetDeclaration(true, true, true), mDebugger_selected_stack == i))
						mDebugger_selected_stack = i;
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("File: %s", stack_info[i].func->GetScriptSectionName());
						ImGui::Text("Line: %d", stack_info[i].line);
						ImGui::EndTooltip();
					}
				}

				ImGui::EndChild();
				ImGui::EndGroup();

				ImGui::SameLine();

				ImGui::BeginGroup();
				ImGui::TextUnformatted("Variables");
				ImGui::BeginChild("varlist", ImVec2(0, 0), true);
				auto var_info = script.get_var_info(mDebugger_selected_thread, mDebugger_selected_stack);
				for (size_t i = 0; i < var_info.size(); i++)
				{
					auto &var = var_info[i];
					ImGui::Columns(2, "colls");

					//ImGui::Selectable(ImGui::IdOnly(std::to_string(i) + "info").c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
					//ImGui::SameLine();
					ImGui::Text("%s %s", var.type_decl, var.name);
					ImGui::NextColumn();
					if (var.type_id == AS::asTYPEID_INT32)
						ImGui::Text("%d", *(int*)var.pointer);
					else if (var.type_id == AS::asTYPEID_FLOAT)
						ImGui::Text("%f", *(float*)var.pointer);
					else if (var.type_id == AS::asTYPEID_BOOL)
						ImGui::Text("%d", *(bool*)var.pointer);
					else if (var.type_id == AS::asTYPEID_UINT32)
						ImGui::Text("%u", *(unsigned int*)var.pointer);
					else if (var.type_id == script.get_type_info_from_decl("string")->GetTypeId())
					{
						if (var.pointer)
							ImGui::Text("%s", ((std::string*)var.pointer)->c_str());
						else
							ImGui::TextUnformatted("<null>");
					}
					else if (var.type_id == script.get_type_info_from_decl("vec")->GetTypeId())
					{
						if (var.pointer)
							ImGui::Text(((engine::fvector*)var.pointer)->to_string().c_str());
						else
							ImGui::TextUnformatted("<null>");
					}
					else if (var.type_id == script.get_type_info_from_decl("entity")->GetTypeId())
					{
						if (var.pointer)
							ImGui::Text("is_valid=%d", ((rpg::entity_reference*)var.pointer)->is_valid());
						else
							ImGui::TextUnformatted("<null>");
					}
					else
						ImGui::TextColored({ 1, 0.5f, 0.5f, 1 }, "Cannot display");
					ImGui::NextColumn();
				}
				ImGui::Columns(1);
				ImGui::EndChild();
				ImGui::EndGroup();
			}
			else
			{
				ImGui::Text("No script to debug");
			}
		}
		ImGui::End();
		
		mWindow.clear();
		ImGui::SFML::Render(mWindow.get_sfml_window());
		mWindow.display();
	}
	ImGui::SFML::Shutdown();
	ImGui::SaveSettings("./editor/settings_ext.xml");
}

void WGE_imgui_editor::place_tile(engine::fvector pos)
{
	if (mTilemap_manipulator.get_layer_count() > 0)
	{
		mIs_scene_modified = true;
		rpg::tile tile;
		tile.set_position(pos);
		tile.set_atlas(mCurrent_tile_atlas);
		tile.set_rotation((unsigned int)mTile_rotation);
		mCommand_manager.execute<command_set_tiles>(mCurrent_layer, mTilemap_manipulator, tile);
		mTilemap_manipulator.get_layer(mCurrent_layer).sort();
		mTilemap_display.update(mTilemap_manipulator);
	}
}

void WGE_imgui_editor::remove_tile(engine::fvector pos)
{
	if (mTilemap_manipulator.get_layer_count() > 0
		&& mTilemap_manipulator.get_layer(mCurrent_layer).find_tile(pos)) // Check if a tile will actually be deleted
	{
		mIs_scene_modified = true;
		mCommand_manager.execute<command_remove_tiles>(mCurrent_layer, mTilemap_manipulator, pos);
		mTilemap_manipulator.get_layer(mCurrent_layer).sort();
		mTilemap_display.update(mTilemap_manipulator);
	}
}

void WGE_imgui_editor::new_box(rpg::collision_box::ptr pBox, engine::fvector pPos)
{
	mCommand_manager.add_new<command_add_wall>(mColl_container, pBox);
	pBox->set_region({ engine::fvector(pPos).floor(), engine::fvector(1, 1) });
	mSelected_collbox = pBox;
	mIs_scene_modified = true;
}

void WGE_imgui_editor::duplicate_box()
{
	auto ncopy = mSelected_collbox->copy();
	mColl_container.add_collision_box(ncopy);
	mCommand_manager.add_new<command_add_wall>(mColl_container, ncopy);
	mSelected_collbox = ncopy;
	mIs_scene_modified = true;
}

void WGE_imgui_editor::delete_box()
{
	mColl_container.remove_box(mSelected_collbox);
	mCommand_manager.add_new<command_remove_wall>(mColl_container, mSelected_collbox);
	mSelected_collbox.reset();
	mIs_scene_modified = true;
}

bool WGE_imgui_editor::is_ready_to_close()
{
	return mIs_closing && !mAtlas_editor.is_confirming_save() && !ImGui::IsPopupOpen("###askforsave");
}

void WGE_imgui_editor::prepare_scene(engine::fs::path pPath, const std::string& pName)
{
	// Load the scene settings
	if (mScene_loader.load(pPath, pName))
	{
		// Load up the new tilemap
		mTilemap_texture = mGame.get_resource_manager().get_resource<engine::texture>("texture"
			, mScene_loader.get_tilemap_texture());
		if (!mTilemap_texture)
		{
			logger::error("Failed to load tilemap texture for editor");
			return;
		}
		mTilemap_manipulator.set_texture(mTilemap_texture);
		mTilemap_manipulator.load_xml(mScene_loader.get_tilemap());
		mTilemap_display.set_texture(mTilemap_texture);
		mTilemap_display.set_unit(static_cast<float>(mTile_size));
		mTilemap_display.update(mTilemap_manipulator);

		mColl_container.load_xml(mScene_loader.get_collisionboxes());
		mSelected_collbox.reset();

		mCurrent_tile_atlas.reset();
		mCurrent_layer = 0;

		center_tilemap();

		mIs_scene_modified = false;
		mCommand_manager.clear();
	}
}

void WGE_imgui_editor::save_scene()
{
	logger::info("Saving scene");
	mTilemap_manipulator.condense_all();
	mScene_loader.set_tilemap(mTilemap_manipulator);
	mScene_loader.set_collisionboxes(mColl_container);
	mTilemap_manipulator.explode_all();
	if (!mScene_loader.save())
	{
		logger::error("Failed saving scene xml file");
		return;
	}
	mIs_scene_modified = false;
	logger::info("Scene saved");
}

void WGE_imgui_editor::draw_scene_window()
{
	if (!mWindow_open[window_open_scene_properties])
		return;
	if (ImGui::Begin("Scene Properties", &mWindow_open[window_open_scene_properties]))
	{
		ImGui::BeginGroup();
		ImGui::Text("Scene List");
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
			mScene_list = mGame.get_scene_list();
		ImGui::QuickTooltip("Repopulate the scene list");

		ImGui::BeginChild("Scenelist", ImVec2(200, -25), true);
		for (auto& i : mScene_list)
			if (ImGui::Selectable(i.c_str(), i == mGame.get_scene().get_path()))
				mGame.get_scene().load_scene(i);
		ImGui::EndChild();

		if (ImGui::Button("New###NewScene"))
		{
			mNew_scene_name.clear();
			mNew_scene_texture_name.clear();
			ImGui::OpenPopup("Create scene");
		}
		ImGui::QuickTooltip("Create a new scene for this game");

		new_scene_popup();

		ImGui::EndGroup();

		//ImGui::SameLine();
		//ImGui::VSplitter("vsplitter", 8, &w);

		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::PushItemWidth(-100);
		if (ImGui::Button("Restart"))
			mGame.get_scene().reload_scene(); // This does not affect the editors
		ImGui::QuickTooltip("Restarts the current scene. This will not affect the editors.");

		ImGui::SameLine();
		if (ImGui::Button("Save"))
			save_scene();

		ImGui::SameLine();
		if (ImGui::Button("Edit Script"))
		{
			std::string cmd = mSettings.generate_open_cmd(mScene_loader.get_script_path());
			if (!util::create_process_cmdline(cmd))
				logger::error("Failed to launch editor");
		}
		ImGui::SameLine();
		if (ImGui::Button("Edit Xml"))
		{
			std::string cmd = mSettings.generate_open_cmd(mScene_loader.get_scene_path());
			if (!util::create_process_cmdline(cmd))
				logger::error("Failed to launch editor");
		}
		ImGui::QuickTooltip("Warning: It is not recommended that you actually edit this file.");

		ImGui::TextUnformatted(("Name: " + mScene_loader.get_name()).c_str());

		if (ImGui::TreeNode("Tilemap"))
		{
			ImGui::TextUnformatted(("Texture: " + mScene_loader.get_tilemap_texture()).c_str());
			if (ImGui::Button("Change Texture"))
			{
				mChange_scene_texture_name = mScene_loader.get_tilemap_texture();
				ImGui::OpenPopup("Change Scene Texture");
			}
			change_texture_popup();
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Boundary"))
		{
			bool has_boundary = mScene_loader.has_boundary();
			if (ImGui::Checkbox("Enabled", &has_boundary))
			{
				mIs_scene_modified = true;
				mScene_loader.set_has_boundary(has_boundary);
			}
			ImGui::QuickTooltip("The boundary is the region in which the camera will be contained. The camera cannot move out of the boundary.");

			engine::frect boundary = mScene_loader.get_boundary();
			if (ImGui::DragFloat4("Dimensions", boundary.components))
			{
				mIs_scene_modified = true;
				mScene_loader.set_boundary(boundary);
			}
			ImGui::QuickTooltip("Sets the boundary's x, y, width, and height respectively. This is in unit tiles.");

			ImGui::TreePop();
		}
		ImGui::PopItemWidth();
		ImGui::EndGroup();
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_game_window()
{
	if (!mWindow_open[window_open_game_window])
		return;
	if (ImGui::Begin("Game", &mWindow_open[window_open_game_window]))
	{
		if (ImGui::Button("Restart game", ImVec2(-0, 0)))
		{
			mGame.restart_game();
		}
		ImGui::SameLine();
		if (ImGui::Button("Open game", ImVec2(-0, 0)))
		{
			ImGui::OpenPopup("Open Game");
		}
		static engine::fs::path game_path;
		if (ImGui::FileOpenerPopup("Open Game", &game_path, true, true))
		{
			mGame.clear_scene();
			mGame.load(game_path);
			mScene_list = mGame.get_scene_list();
		}

		ImGui::PushItemWidth(-100);

		if (ImGui::TreeNode("Game"))
		{
			static char game_name_buffer[256]; // Temp
			ImGui::InputText("Name", &game_name_buffer[0], 256);
			ImGui::QuickTooltip("Name of this game.\nThis is displayed in the window title.");

			ImGui::InputInt("Tile Size", &mTile_size, 1, 2);
			ImGui::QuickTooltip("Represents both the width and height of the tiles.");
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Display"))
		{
			static int target_size[2] = { 384 , 320 }; // Temp
			ImGui::DragInt2("Target Size", target_size);

			static int window_size[2] = { 384 , 320 }; // Temp
			ImGui::DragInt2("Window Size", window_size);

			static bool resizable = true; // temp
			ImGui::Checkbox("Resizable", &resizable);
			ImGui::TreePop();
		}
		ImGui::PopItemWidth();
		if (ImGui::TreeNode("Flags"))
		{
			// TODO: Add Filtering
			ImGui::BeginChild("Flag List", ImVec2(0, 100), true);
			for (size_t i = 0; i < 5; i++)
			{
				ImGui::Selectable(("Flag " + std::to_string(i)).c_str());
				if (ImGui::BeginPopupContextItem(("flagcxt" + std::to_string(i)).c_str()))
				{
					ImGui::MenuItem("Unset");
					ImGui::EndPopup();
				}
			}
			ImGui::EndChild();
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Resources"))
		{
			ImGui::BeginChild("Resource List", ImVec2(0, 200), true);

			ImGui::Columns(3);

			ImGui::SetColumnWidth(0, 100);
			ImGui::TextUnformatted("Status");
			ImGui::NextColumn();

			ImGui::TextUnformatted("Type");
			ImGui::NextColumn();

			ImGui::TextUnformatted("Name");
			ImGui::NextColumn();
			ImGui::Separator();

			for (auto& i : mGame.get_resource_manager().get_resources())
			{
				ImGui::Selectable(i->is_loaded() ? "Loaded" : "Not Loaded", false, ImGuiSelectableFlags_SpanAllColumns);
				ImGui::NextColumn();

				ImGui::TextUnformatted(i->get_type().c_str());
				ImGui::NextColumn();

				ImGui::TextUnformatted(i->get_name().c_str());
				ImGui::NextColumn();
			}
			ImGui::EndChild();
			ImGui::Columns(1);

			if (ImGui::Button("Reload All"))
			{
				mGame.get_resource_manager().reload_all();
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_game_view_window()
{
	if (ImGui::Begin("Game View"))
	{
		// This game will not recieve events if this is false
		mIs_game_view_window_focused = ImGui::IsWindowFocused();

		if (resize_to_window(mGame_render_target))
			mGame_renderer.refresh();

		mGame.tick();

		// Render the game
		mGame_renderer.draw();
		mGame_render_target.display();

		// Display on imgui window. 
		ImGui::AddBackgroundImage(mGame_render_target);
		
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(sf::Keyboard::F1))
			mShow_debug_info = !mShow_debug_info;

		if (mShow_debug_info)
		{
			const engine::fvector window_mouse_position = static_cast<engine::fvector>(ImGui::GetMousePos()) - static_cast<engine::fvector>(ImGui::GetCursorScreenPos());
			const engine::fvector view_mouse_position = mGame_renderer.window_to_game_coords(engine::vector_cast<int>(window_mouse_position));
			const engine::fvector view_tile_mouse_position = view_mouse_position / (float)mTile_size;
			const engine::fvector tile_mouse_position = mGame_renderer.window_to_game_coords(engine::vector_cast<int>(window_mouse_position), mGame.get_scene().get_world_node());
			
			ImDrawList * dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(ImGui::GetCursorScreenPos(), static_cast<engine::fvector>(ImGui::GetCursorScreenPos()) + engine::fvector(300, ImGui::GetTextLineHeightWithSpacing() * 6), engine::color(0, 0, 0, 150).to_uint32(), 5);

			ImGui::BeginGroup();
			ImGui::Text("FPS: %.2f", mGame_renderer.get_fps());
			ImGui::Text("Frame: %.2fms", mGame_renderer.get_delta() * 1000);
			ImGui::Text("Mouse position:");
			ImGui::Text("        (View)  %.2f, %.2f pixels", view_mouse_position.x, view_mouse_position.y);
			ImGui::Text("                %.2f, %.2f tiles", view_tile_mouse_position.x, view_tile_mouse_position.y);
			ImGui::Text("       (World)  %.2f, %.2f tiles", tile_mouse_position.x, tile_mouse_position.y);
			ImGui::EndGroup();
			ImGui::QuickTooltip("Basic debug info.\nUse F1 to toggle.");
		}
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_scene_editor_window()
{
	if (!mWindow_open[window_open_scene_editor])
		return;
	if (ImGui::Begin("Scene Editor", &mWindow_open[window_open_scene_editor]))
	{
		ImGui::BeginRenderer("view", mScene_editor_rendererdata, ImVec2(-300, 0), ImGuiRendererFlags_Editor);
		ImGui::SetRendererUnit(static_cast<float>(mTile_size));

		mTilemap_display.draw(ImGui::GetCurrentRenderer());

		// Update the editors
		if (mCurrent_scene_editor == editor_tilemap)
			tilemap_editor_update();
		else if (mCurrent_scene_editor == editor_collision)
			collision_editor_update();
		
		if (mShow_grid)
			ImGui::RenderWidgets::Grid(mGrid_color);

		ImGui::EndRenderer();
		ImGui::SameLine();
		ImGui::BeginGroup();

		ImGui::RadioButton("Tilemap", &mCurrent_scene_editor, editor_tilemap);
		ImGui::SameLine();
		ImGui::RadioButton("Collision", &mCurrent_scene_editor, editor_collision);

		if (ImGui::TreeNodeEx("Visual", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(-100);
			const char* snapping_items[] = { "None", "Pixel", "4x4", "2x2", "1x1" };
			ImGui::Combo("Snapping", &mTilemap_current_snapping, snapping_items, 5);

			if (ImGui::Button("Focus on..."))
				ImGui::OpenPopup("FocusOnPopup");
			if (ImGui::BeginPopup("FocusOnPopup"))
			{
				if (ImGui::MenuItem("Center of Tilemap"))
					center_tilemap();
				if (ImGui::MenuItem("Camera Focus"))
				{
					ImGui::UseRenderer(mScene_editor_rendererdata);
					ImGui::SetRendererPan(mGame.get_scene().get_camera_focus());
					ImGui::EndRenderer();
				}
				ImGui::EndPopup();
			}

			ImGui::Checkbox("grid", &mShow_grid);

			ImGui::ColorEdit3("Grid Color", mGrid_color.components);

			ImGui::UseRenderer(mScene_editor_rendererdata);
			engine::color bg = ImGui::GetRendererBackground();
			if (ImGui::ColorEdit3("Bg Color", bg.components))
				ImGui::SetRendererBackground(bg);
			ImGui::EndRenderer();

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}

		// Draw the settings for the editors
		if (mCurrent_scene_editor == editor_tilemap)
			draw_tilemap_editor_settings();
		else if (mCurrent_scene_editor = editor_collision)
			draw_collision_editor_settings();
		ImGui::EndGroup();
	}
	ImGui::End();
}

static inline void draw_door_offset_vector(engine::primitive_builder& pPrimitives, const rpg::collision_box::ptr& pCB, int pTile_size)
{
	auto door = std::dynamic_pointer_cast<rpg::door>(pCB);
	const engine::fvector center = door->get_region().get_center()*(float)pTile_size;
	const engine::fvector offset = door->get_offset()*(float)pTile_size + center;
	pPrimitives.add_line(center, offset, { 1, 0.7f, 0.7f, 1 });
	pPrimitives.add_circle(offset, 5, engine::color_preset::transparent, { 1, 0.7f, 0.7f, 1 });
}

void WGE_imgui_editor::collision_editor_update()
{
	engine::primitive_builder& primitives = ImGui::GetRendererPrimitives();

	// Darken the tilemap and anything else in the background.
	// This allows the editor-specific objects to stand out more.
	primitives.add_rectangle({ engine::fvector(0, 0), ImGui::GetRendererSize() }, { 0, 0, 0, 0.4f });

	primitives.push_node(mTilemap_display);
	for (auto& i : mColl_container.get_boxes())
	{
		engine::color outline;
		if (i == mSelected_collbox)
			outline = { 1, 1, 0.5f, 1 }; // Selected
		else if (i->get_region().is_intersect(ImGui::GetRendererWorldMouse()))
			outline = { 1, 1, 1, 1 }; // Mouse hover
		else
			outline = { 0.4f, 1, 0.4f, 1 }; // Not hovering
		primitives.add_rectangle(i->get_region()*(float)mTile_size, { 0.7f, 1, 0.7f, 0.7f }, outline);

		// Draw door player offset vector
		if (i->get_type() == rpg::collision_box::type::door)
			draw_door_offset_vector(primitives, i, mTile_size);
	}
	primitives.pop_node();

	// Draw draggers for resizing selected collisionbox
	if (mSelected_collbox)
	{
		if (ImGui::RenderWidgets::RectDraggerAndResizer("collbox_resize", mSelected_collbox->get_region(), &mBox_change))
		{
			if (ImGui::IsItemClicked(0)) // Begin some dragging
			{
				mBox_change = engine::frect(0, 0, 0, 0);
				mOriginal_box = mSelected_collbox->get_region();
				mCommand_manager.start<command_wall_changed>(mSelected_collbox);
			}
			engine::frect new_box_rect = mOriginal_box + snap_closest(mBox_change, calc_snapping(mTilemap_current_snapping, mTile_size));
			new_box_rect.w = std::max(new_box_rect.w, 1.f / mTile_size); // Limit the size to one pixel so you can still see and select it
			new_box_rect.h = std::max(new_box_rect.h, 1.f / mTile_size);
			mSelected_collbox->set_region(new_box_rect);
		
			if (ImGui::IsMouseReleased(0)) // End dragging
			{
				if (mBox_change == engine::frect(0, 0, 0, 0))
					mCommand_manager.cancel(); // There was no change so dont add it the history
				else
				{
					mCommand_manager.complete();
					mIs_scene_modified = true;
				}
			}
		}
	}

	// Collision box selection
	// Placed last so the draggers get priority
	// TODO: Some way to select box behind the currently select box
	if (!ImGui::RenderWidgets::IsDragging() && (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)))
	{
		auto hits = mColl_container.collision(ImGui::GetRendererWorldMouse());
		if (!hits.empty())
			mSelected_collbox = hits.back(); // Select the top
		else
			mSelected_collbox.reset();
	}

	// Right click options
	if (!ImGui::RenderWidgets::IsDragging() && ImGui::IsItemClicked(1))
		ImGui::OpenPopup("coll_box_options");
	if (ImGui::BeginPopup("coll_box_options"))
	{
		if (ImGui::BeginMenu("New..."))
		{
			if (ImGui::MenuItem("Wall")) new_box(mColl_container.add_wall(), ImGui::GetRendererWorldMouse());
			if (ImGui::MenuItem("Button")) new_box(mColl_container.add_button(), ImGui::GetRendererWorldMouse());
			if (ImGui::MenuItem("Trigger")) new_box(mColl_container.add_trigger(), ImGui::GetRendererWorldMouse());
			if (ImGui::MenuItem("Door")) new_box(mColl_container.add_door(), ImGui::GetRendererWorldMouse());
			ImGui::EndMenu();
		}

		if (mSelected_collbox)
		{
			ImGui::Separator();
			if (ImGui::MenuItem("Duplicate"))
				duplicate_box();
			if (ImGui::MenuItem("Delete"))
				delete_box();
		}
		ImGui::EndPopup();
	}
}

void WGE_imgui_editor::draw_collision_editor_settings()
{
	ImGui::PushItemWidth(-100);
	if (mSelected_collbox && ImGui::TreeNodeEx("Collision Box", ImGuiTreeNodeFlags_DefaultOpen))
	{
		int current_type = static_cast<int>(mSelected_collbox->get_type());
		const char* coll_type_items[] = { "Wall", "Trigger", "Button", "Door" };
		ImGui::Combo("Type", &current_type, coll_type_items, 4);

		std::string group_buf;
		if (mSelected_collbox->get_wall_group())
			group_buf = mSelected_collbox->get_wall_group()->get_name();
		if (ImGui::InputText("Group", &group_buf, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (group_buf.length() == 0)
				mSelected_collbox->set_wall_group(nullptr);
			else
				mSelected_collbox->set_wall_group(mColl_container.create_group(group_buf));
			mIs_scene_modified = true;
		}

		ImGui::QuickTooltip("Group to assosiate this collision box with.\nThis is used in scripts to enable/disable boxes or calling functions when collided.");

		engine::frect coll_rect = mSelected_collbox->get_region();
		if (ImGui::DragFloat4("Rect", coll_rect.components, 1))
		{
			mSelected_collbox->set_region(coll_rect);
			mIs_scene_modified = true;
		}

		ImGui::TreePop();
	}

	if (mSelected_collbox
		&& mSelected_collbox->get_type() == rpg::collision_box::type::door
		&& ImGui::TreeNodeEx("Door", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto door = std::dynamic_pointer_cast<rpg::door>(mSelected_collbox);

		std::string door_name_buf(door->get_name());
		if (ImGui::InputText("Name", &door_name_buf))
		{
			door->set_name(door_name_buf);
			mIs_scene_modified = true;
		}

		engine::fvector noffset = door->get_offset();
		if (ImGui::DragFloat2("Offset", noffset.components, 0.1f))
		{
			door->set_offset(noffset);
			mIs_scene_modified = true;
		}
		ImGui::QuickTooltip("This will offset the player when they come through this door.\nUsed to prevent player from colliding with the door again and going back.");

		// TODO: Use a combo instead
		if (ImGui::BeginCombo("Dest. Scene", door->get_scene().empty() ? "No Scene" : door->get_scene().c_str()))
		{
			for (auto& i : mScene_list)
			{
				if (ImGui::Selectable(i.c_str(), door->get_scene() == i))
				{
					door->set_scene(i);
					mIs_scene_modified = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::QuickTooltip("The scene to load when the player enters this door.");

		// TODO: Use a combo instead
		std::string door_dest_door_buf(door->get_destination());
		if (ImGui::InputText("Dest. Door", &door_dest_door_buf))
		{
			door->set_destination(door_dest_door_buf);
			mIs_scene_modified = true;
		}
		ImGui::QuickTooltip("The destination door in the new loaded scene.");

		ImGui::TreePop();
	}
	ImGui::PopItemWidth();
}

void WGE_imgui_editor::tilemap_editor_update()
{
	engine::primitive_builder& primitives = ImGui::GetRendererPrimitives();
	const engine::fvector mouse_snapped = snap(ImGui::GetRendererWorldMouse(), calc_snapping(mTilemap_current_snapping, mTile_size));
	if (mTilemap_texture && mCurrent_tile_atlas)
	{
		if (ImGui::GetIO().KeyCtrl)
		{
			if (ImGui::IsItemClicked())
			{
				// Copy tile
				if (auto tile = mTilemap_manipulator.get_layer(mCurrent_layer).find_tile(mouse_snapped))
				{
					mCurrent_tile_atlas = tile->get_atlas();
					mTile_rotation = tile->get_rotation();
				}
			}
		}
		else
		{
			const bool left_mouse_down = ImGui::IsMouseDown(0) && ImGui::IsItemHovered();
			const bool right_mouse_down = ImGui::IsMouseDown(1) && ImGui::IsItemHovered();
			if (left_mouse_down || right_mouse_down) // Allows user to "paint" tiles
			{
				const bool is_new_position = mLast_tile_position != mouse_snapped;
				if (ImGui::IsItemClicked() || (left_mouse_down && is_new_position))
					place_tile(mouse_snapped);
				else if (ImGui::IsItemClicked(1) || (right_mouse_down && is_new_position))
					remove_tile(mouse_snapped);
				mLast_tile_position = mouse_snapped;
			}
		}
	}

	// Draw previewed tile
	if (mTilemap_texture && mCurrent_tile_atlas && mTilemap_manipulator.get_layer_count() > 0)
	{
		primitives.push_node(mTilemap_display);

		// Highlight the tile that may be replaced/removed
		if (auto tile = mTilemap_manipulator.get_layer(mCurrent_layer).find_tile(mouse_snapped))
		{
			engine::fvector size = tile->get_atlas()->get_root_frame().get_size();
			primitives.add_rectangle({ tile->get_position()*(float)mTile_size, tile->get_rotation() % 2 ? size.flip() : size }
			, { 1, 0.5f, 0.5f, 0.5f }, { 1, 0, 0, 0.7f });
		}

		// Draw the tile texture that the user will place
		primitives.add_quad_texture(mTilemap_texture, mouse_snapped*(float)mTile_size
			, mCurrent_tile_atlas->get_root_frame(), { 1, 1, 1, 0.7f }, mTile_rotation);

		primitives.pop_node();
	}
}

void WGE_imgui_editor::draw_tilemap_editor_settings()
{
	static float tilegroup_height = 300; // FIXME: make nonstatic
	if (ImGui::TreeNodeEx("Tile", ImGuiTreeNodeFlags_DefaultOpen))
	{
		draw_tile_group(tilegroup_height);
		ImGui::HSplitter("tileandlayerssplitter", 3, &tilegroup_height, true);
		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("Layers", ImGuiTreeNodeFlags_DefaultOpen))
	{
		draw_tilemap_layers_group();
		ImGui::TreePop();
	}
}

inline static void draw_tile_preview(engine::fvector pSize, std::shared_ptr<engine::texture> pTexture, engine::subtexture::ptr pAtlas)
{
	// Scale the preview image to fit the window while maintaining aspect ratio
	engine::fvector preview_size = pSize - engine::fvector(ImGui::GetStyle().WindowPadding) * 2;
	engine::fvector size = pAtlas->get_root_frame().get_size();
	engine::fvector scaled_size =
	{
		std::min(size.x*(preview_size.y / size.y), preview_size.x),
		std::min(size.y*(preview_size.x / size.x), preview_size.y)
	};
	ImGui::SetCursorPos(preview_size / 2 - scaled_size / 2 + ImGui::GetStyle().WindowPadding); // Center it
	ImGui::Image(pTexture->get_sfml_texture(), scaled_size, pAtlas->get_root_frame()); // Draw it
}

void WGE_imgui_editor::draw_tile_group(float from_bottom)
{
	ImGui::BeginChild("Tilesettingsgroup", ImVec2(0, -from_bottom));

	ImGui::BeginChild("Tile Preview", ImVec2(100, 100), true);
	if (mTilemap_texture && mCurrent_tile_atlas)
		draw_tile_preview({ 100, 100 }, mTilemap_texture, mCurrent_tile_atlas);
	else
		ImGui::TextUnformatted("No preview");
	ImGui::EndChild();
	ImGui::QuickTooltip("Preview of tile to place.");
	//TODO: Add option to change background color of preview in case the texture is hard to see

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::PushItemWidth(-100);

	ImGui::DragInt("Rotation", &mTile_rotation, 0.2f, 0, 3, u8"%.0f x90\u00B0");

	ImGui::PopItemWidth();
	ImGui::EndGroup();

	ImGui::BeginChild("Tile List", ImVec2(0, 0), true);
	if (mTilemap_texture)
	{
		for (const auto& i : mTilemap_texture->get_texture_atlas().get_all())
		{
			// Possibly add a preview for each tile in the list, but that's for another day
			if (ImGui::Selectable(i->get_name().c_str(), mCurrent_tile_atlas == i))
				mCurrent_tile_atlas = i;
		}
	}
	else
		ImGui::TextUnformatted("No texture");
	ImGui::EndChild();
	ImGui::EndChild();
}

void WGE_imgui_editor::draw_tilemap_layers_group()
{
	ImGui::BeginGroup();
	ImGui::BeginChild("Layer List", ImVec2(0, -25), true);
	static bool layer_visible = false;
	ImGui::Columns(2, 0, false);
	ImGui::SetColumnWidth(0, 25);
	const size_t layer_count = mTilemap_manipulator.get_layer_count();
	for (size_t i = 0; i < layer_count; i++)
	{
		const size_t layer_index = layer_count - i - 1; // Reverse so the top layer is at the top of the list
		rpg::tilemap_layer& layer = mTilemap_manipulator.get_layer(layer_index);

		// Checkbox on left that enables or disables a layer
		bool is_visible = mTilemap_display.is_layer_visible(layer_index);
		if (ImGui::Checkbox(ImGui::IdOnly("Tilemaplayer" + std::to_string(layer_index)).c_str(), &is_visible))
			mTilemap_display.set_layer_visible(layer_index, is_visible);
		ImGui::NextColumn();

		if (ImGui::Selectable(ImGui::NameId(layer.get_name(), "layer" + std::to_string(layer_index)).c_str(), mCurrent_layer == layer_index))
		{
			mCurrent_layer = layer_index;
			if (ImGui::IsMouseDoubleClicked(0))
				ImGui::OpenPopup("Rename Layer");
		}
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::Separator();

	if (ImGui::Button("New"))
	{
		mCommand_manager.execute<command_add_layer>(mTilemap_manipulator, mCurrent_layer);
		mTilemap_display.update(mTilemap_manipulator);
	}

	ImGui::SameLine();
	if (ImGui::Button("Rename"))
		ImGui::OpenPopup("Rename Layer");
	if (ImGui::BeginPopupModal("Rename Layer", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char name[256]; // TODO: default to original name
		bool return_pressed = ImGui::InputText("Name", name, 256, ImGuiInputTextFlags_EnterReturnsTrue);
		bool renamebt_pressed = ImGui::Button("Rename", ImVec2(100, 25));
		if ((return_pressed || renamebt_pressed) && strnlen(name, sizeof(name)/sizeof(name[0])) != 0)
		{
			mTilemap_manipulator.get_layer(mCurrent_layer).set_name(name);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 25)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete") && mTilemap_manipulator.get_layer_count() > 0)
		ImGui::OpenPopup("Confirm layer removal");

	if (mTilemap_manipulator.get_layer_count() > 0
		&& ImGui::ConfirmPopup("Confirm layer removal"
		, ("Are you sure you want to remove layer \""
			+ mTilemap_manipulator.get_layer(mCurrent_layer).get_name() + "\"?").c_str()) == 1)
	{
		mCommand_manager.execute<command_delete_layer>(mTilemap_manipulator, mCurrent_layer);
		if (mTilemap_manipulator.get_layer_count() == 0)
			mCurrent_layer = 0;
		else
			mCurrent_layer = std::min(mCurrent_layer, mTilemap_manipulator.get_layer_count() - 1);
		mTilemap_display.update(mTilemap_manipulator);
	}

	ImGui::SameLine();
	if (ImGui::ArrowButton("Move Up", ImGuiDir_Up) && mCurrent_layer != mTilemap_manipulator.get_layer_count() - 1)
	{
		mCommand_manager.execute<command_move_layer>(mTilemap_manipulator, mCurrent_layer, mCurrent_layer + 1);
		++mCurrent_layer;
		mTilemap_display.update(mTilemap_manipulator);
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("Move Down", ImGuiDir_Down) && mCurrent_layer != 0)
	{
		mCommand_manager.execute<command_move_layer>(mTilemap_manipulator, mCurrent_layer, mCurrent_layer - 1);
		--mCurrent_layer;
		mTilemap_display.update(mTilemap_manipulator);
	}
	ImGui::EndGroup();
}

void WGE_imgui_editor::center_tilemap()
{
	ImGui::UseRenderer(mScene_editor_rendererdata);
	ImGui::SetRendererPan(mTilemap_manipulator.get_center_point());
	ImGui::EndRenderer();
}

void WGE_imgui_editor::draw_settings_window()
{
	if (!mWindow_open[window_open_settings])
		return;
	if (ImGui::Begin("Settings", &mWindow_open[window_open_settings]))
	{
		if (ImGui::CollapsingHeader("Scene Editor"))
		{
			float cdsize = 3;
			ImGui::GetSetting("Circle Dragger Radius", &cdsize);
			ImGui::DragFloat("Circle Dragger Radius", &cdsize);
			ImGui::UpdateSetting("Circle Dragger Radius", &cdsize);
		}
	}
	ImGui::End();
}

void WGE_imgui_editor::draw_log_window()
{
	if (!mWindow_open[window_open_log])
		return;
	if (ImGui::Begin("Log", &mWindow_open[window_open_log]))
	{
		const auto& log = logger::get_log();

		// Helps keep track of changes in the log
		static size_t last_log_size = 0;

		// If the window is scrolled to the bottom, maintain it at the bottom
		// To prevent it from locking the users mousewheel input, it will only lock the scroll
		// when the log actually changes.
		bool lock_scroll_at_bottom = ImGui::GetScrollY() == ImGui::GetScrollMaxY() && last_log_size != log.size();

		ImGui::Columns(2, 0, false);
		ImGui::SetColumnWidth(0, 60);
		const size_t start = log.size() >= 256 ? log.size() - 256 : 0; // Limit to 256
		for (size_t i = start; i < log.size(); i++)
		{
			switch (log[i].type)
			{
			case logger::level::info:
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
				ImGui::TextUnformatted("Info");
				break;
			case logger::level::debug:
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 1, 1, 1 }); // Cyan-ish
				ImGui::TextUnformatted("Debug");
				break;
			case logger::level::warning:
				ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 0.5f, 1 }); // Yellow-ish
				ImGui::TextUnformatted("Warning");
				break;
			case logger::level::error:
				ImGui::PushStyleColor(ImGuiCol_Text, { 1, 0.5f, 0.5f, 1 }); // Red
				ImGui::TextUnformatted("Error");
				break;
			}
			ImGui::NextColumn();

			// The actual message. Has the same color as the message type.
			ImGui::TextUnformatted(log[i].msg.c_str());
			ImGui::PopStyleColor();

			if (log[i].is_file)
			{
				std::string file_info = log[i].file;
				if (log[i].row >= 0) // Line info
				{
					file_info += " (" + std::to_string(log[i].row);
					if (log[i].column >= 0) // Column info
						file_info += ", " + std::to_string(log[i].column);
					file_info += ")";
				}

				// Filepath. Gray.
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.7f, 0.7f, 0.7f, 1 });
				if (ImGui::HiddenSmallButton(file_info.c_str()))
				{
					std::string cmd = mSettings.generate_open_cmd(log[i].file);
					if (!util::create_process_cmdline(cmd))
						logger::error("Failed to launch editor");
				}
				ImGui::QuickTooltip("Open file in editor.");
				ImGui::PopStyleColor();
			}
			ImGui::NextColumn();
		}
		ImGui::Columns(1);

		if (lock_scroll_at_bottom)
			ImGui::SetScrollHere();
		last_log_size = log.size();
	}
	ImGui::End();
}

void WGE_imgui_editor::handle_undo_redo()
{
	if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
		return;
	// Undo
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(sf::Keyboard::Z))
	{
		if (mCommand_manager.undo())
		{
			mTilemap_manipulator.get_layer(mCurrent_layer).sort();
			mTilemap_display.update(mTilemap_manipulator);
		}
	}

	// Redo
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(sf::Keyboard::Y))
	{
		if (mCommand_manager.redo())
		{
			mTilemap_manipulator.get_layer(mCurrent_layer).sort();
			mTilemap_display.update(mTilemap_manipulator);
		}
	}
}

void WGE_imgui_editor::handle_save_confirmations()
{
	// Has the current scene changed?
	if (mScene_loader.get_name() != mGame.get_scene().get_name() && !ImGui::IsPopupOpen("###askforsave"))
	{
		if (mIs_scene_modified)
			ImGui::OpenPopup("###askforsave");
		else
			prepare_scene(mGame.get_data_path() / rpg::defs::DEFAULT_SCENES_PATH, mGame.get_scene().get_name());
	}

	if (mIs_closing && mIs_scene_modified && !ImGui::IsPopupOpen("###askforsave"))
		ImGui::OpenPopup("###askforsave");

	// Ask for scene save
	int scene_save_answer = ImGui::ConfirmPopup("Save?###askforsave", "Do you want to save this scene before moving on?");
	if (scene_save_answer == 1) // yes
	{
		save_scene();
		prepare_scene(mGame.get_data_path() / rpg::defs::DEFAULT_SCENES_PATH, mGame.get_scene().get_name());
	}
	else if (scene_save_answer == 2) // no
	{
		// No save
		prepare_scene(mGame.get_data_path() / rpg::defs::DEFAULT_SCENES_PATH, mGame.get_scene().get_name());
	}

	// Run confirmation for atlas editor is the window is closing
	if (mIs_closing && !ImGui::IsPopupOpen("###askforsave"))
		mAtlas_editor.confirm_save();

	if (is_ready_to_close())
		mRunning = false;
}

void WGE_imgui_editor::new_scene_popup()
{
	if (ImGui::BeginPopupModal("Create scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Name", &mNew_scene_name);

		ImGui::TextureSelectCombo("Tilemap Texture", mGame.get_resource_manager(), &mNew_scene_texture_name);

		if (ImGui::Button("Create", ImVec2(100, 25)) && !mNew_scene_name.empty())
		{
			if (mGame.create_scene(mNew_scene_name, mNew_scene_texture_name))
			{
				logger::info("Scene \"" + mNew_scene_name + "\" created");
				mGame.get_scene().load_scene(mNew_scene_name);
			}
			else
				logger::error("Failed to create scene \"" + mNew_scene_name + "\"");
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 25)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void WGE_imgui_editor::change_texture_popup()
{
	if (ImGui::BeginPopupModal("Change Scene Texture", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored({ 1, 1, 0.5f, 1 }, "Warning: This will delete your current tilemap.");
		ImGui::TextureSelectCombo("Tilemap Texture", mGame.get_resource_manager(), &mChange_scene_texture_name);

		if (ImGui::Button("Change", ImVec2(100, 25)))
		{
			mTilemap_manipulator.clear();
			mCurrent_tile_atlas.reset();
			mTilemap_texture = mGame.get_resource_manager().get_resource<engine::texture>(engine::texture_restype, mChange_scene_texture_name);
			mTilemap_manipulator.set_texture(mTilemap_texture);
			mTilemap_display.set_texture(mTilemap_texture);
			mTilemap_display.update(mTilemap_manipulator);
			mCommand_manager.clear();
			mCurrent_layer = 0;
			mScene_loader.set_tilemap_texture(mChange_scene_texture_name);
			mIs_scene_modified = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 25)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

engine::fvector WGE_imgui_editor::calc_snapping(int pSnapping, int pTile_size)
{
	switch (pSnapping)
	{
	default:
	case snapping_none:    return { 0, 0 };
	case snapping_pixel:   return engine::fvector(1, 1) / (float)pTile_size;
	case snapping_4x4:   return { 0.25f, 0.25f };
	case snapping_2x2: return { 0.5f, 0.5f };
	case snapping_1x1:    return { 1, 1 };
	}
}

atlas_imgui_editor::atlas_imgui_editor()
{
	ImGui::OpenRenderer(&mFull_texture_renderdata);
	ImGui::OpenRenderer(&mSubtexture_renderdata);
	mWindow_open = nullptr;
	mCurrent_frame = 0;
	mIs_playing = false;
	mModified = false;
}

atlas_imgui_editor::~atlas_imgui_editor()
{
	ImGui::CloseRenderer(&mFull_texture_renderdata);
	ImGui::CloseRenderer(&mSubtexture_renderdata);
}

void atlas_imgui_editor::request_open_texture(const std::string pName)
{
	mReq_texture_name = pName;
}

void atlas_imgui_editor::update()
{
	if (!mReq_texture_name.empty())
		confirm_save();

	int ans = ImGui::ConfirmPopup("Save atlas?###confirmsave", "You want to save this texture atlas before moving on?");
	if (ans == 1)
		save();
	if (ans == 2 && mReq_texture_name.empty())
		mModified = false; // Signal the main editor that it is ready to close

	if (!mReq_texture_name.empty() && (!mModified || ans != 0))
		open_requested();

	if (!*mWindow_open)
		return;
	ImGui::Begin("Texture Atlas Editor", mWindow_open);

	static float tlb_w = 150;

	ImGui::BeginChild("textureprops", ImVec2(tlb_w, 0));
	if (mTexture)
	{
		if (ImGui::Button("Save", ImVec2(-1, 0)))
			save();
		if (ImGui::Button("Reload", ImVec2(-1, 0)))
			request_open_texture(mTexture->get_name());
		if (ImGui::Button("Generate", ImVec2(-1, 0)))
		{
			mTexture->generate_padded_texture(mResource_manager->get_directory() / "cache");
		}
	}
	ImGui::TextUnformatted("Textures");
	ImGui::BeginChild("texturelist", ImVec2(tlb_w, 0), true);
	for (auto& i : mResource_manager->get_resources_with_type(engine::texture_restype))
		if (ImGui::Selectable(i->get_name().c_str(), mTexture && mTexture->get_name() == i->get_name()))
			request_open_texture(i->get_name());
	ImGui::EndChild();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::VSplitter("texturelbsplitter", 3, &tlb_w);

	ImGui::SameLine();

	update_animation();

	bool do_center_subtexture_preview = false;
	ImGui::BeginRenderer("Texturerenderer", mFull_texture_renderdata, ImVec2(-400, 0), ImGuiRendererFlags_Editor);
	{
		engine::primitive_builder& primitives = ImGui::GetRendererPrimitives();

		if (mTexture)
		{
			primitives.push_node(ImGui::GetRendererWorldNode());
			primitives.add_quad_texture(mTexture, { 0, 0 });

			for (auto& i : mTexture->get_texture_atlas().get_all())
			{
				primitives.add_rectangle(i->get_root_frame(), { 0.6f, 1, 0, 0.4f }, { 1, 1, 1, 1 });

				for (engine::frame_t frame = 1; frame < i->get_frame_count(); frame++)
					primitives.add_rectangle(i->get_frame_at(frame), { 1, 1, 1, 0.4f }, { 1, 1, 1, 0.5f });
			}

			// Texture perimeter
			primitives.add_rectangle({ engine::fvector(0, 0), mTexture->get_size() }, { 0, 0, 0, 0 }, { 0, 1, 1, 1 });
			primitives.pop_node();
		}

		if (mTexture && mSubtexture)
		{
			if (ImGui::RenderWidgets::RectDraggerAndResizer("subtextureresizer", mSubtexture->get_root_frame(), &mChange_rect))
			{
				if (ImGui::IsItemClicked(0))
				{
					mChange_rect = engine::frect(0, 0, 0, 0);
					mOriginal_rect = mSubtexture->get_root_frame();
				}
				engine::frect new_rect = mOriginal_rect + engine::frect(mChange_rect).round();
				mSubtexture->set_frame_rect(new_rect);
				if (!ImGui::IsMouseDown(0))
				{
					if (new_rect != mOriginal_rect)
						mModified = true;
				}
			}
		}

		if (mTexture && !ImGui::RenderWidgets::IsDragging() && ImGui::IsItemClicked(0))
		{
			for (auto& i : mTexture->get_texture_atlas().get_all())
			{
				if (i->get_root_frame().is_intersect(ImGui::GetRendererWorldMouse()))
				{
					mSubtexture = i;
					do_center_subtexture_preview = true;
				}
			}
		}

		if (ImGui::GetRendererZoom() >= 2) // The pixels will be 4 times as big at this zoom
			ImGui::RenderWidgets::Grid({ 1, 1, 1, std::min(0.4f, (ImGui::GetRendererZoom() / 4.f) * 0.4f) }); // Adjust alpha so it doesn't pop in too intrusively
	}
	ImGui::EndRenderer();

	if (do_center_subtexture_preview)
		center_subtexture_preview();

	ImGui::SameLine();

	ImGui::BeginChild("Settings", ImVec2(0, 0), true);
	if (mSubtexture)
	{
		ImGui::BeginRenderer("subtexturepreview", mSubtexture_renderdata, ImVec2(0, 300), ImGuiRendererFlags_Editor);
		{
			engine::primitive_builder& primitives = ImGui::GetRendererPrimitives();
			if (mSubtexture)
			{
				primitives.push_node(ImGui::GetRendererWorldNode());

				engine::frect frame;
				frame = mSubtexture->get_frame_at(mCurrent_frame);
				primitives.add_quad_texture(mTexture, engine::fvector(0, 0), mSubtexture->get_frame_at(mCurrent_frame));

				// Subtexture perimeter
				primitives.add_rectangle({ engine::fvector(0, 0), mSubtexture->get_frame_at(0).get_size() }, { 0, 0, 0, 0 }, { 0, 1, 1, 1 });

				primitives.pop_node();
			}
		}
		ImGui::EndRenderer();

		if (mIs_playing && ImGui::Button("Stop"))
			stop_animation();
		else if (!mIs_playing && ImGui::Button("Play"))
			play_animation();

		ImGui::SliderInt("Frame", &mCurrent_frame, 0, mSubtexture->get_frame_count() - 1);
		if (ImGui::IsItemClicked(1))
			ImGui::OpenPopup("gotoframe");
		if (ImGui::BeginPopup("gotoframe"))
		{
			ImGui::InputInt("Goto frame", &mCurrent_frame, 0, mSubtexture->get_frame_count() - 1);
			mCurrent_frame = std::max(mCurrent_frame, 0);
			ImGui::EndPopup();
		}

		int defframe = mSubtexture->get_default_frame();
		if (ImGui::SliderInt("Default Frame", &defframe, 0, mSubtexture->get_frame_count() - 1))
		{
			mSubtexture->set_default_frame(defframe);
			mModified = true;
		}

		int framecount = mSubtexture->get_frame_count();
		if (ImGui::InputInt("Frame Count", &framecount))
		{
			mSubtexture->set_frame_count(std::max(framecount, 1));
			mModified = true;
		}

		float interval = mSubtexture->get_interval();
		if (ImGui::DragFloat("Interval", &interval, 1.0f, 0, 2000, "%.0fms"))
		{
			mSubtexture->add_interval(0, interval);
			mModified = true;
		}

		const char* const loops[] = { "None", "Loop end", "Pingpong" };
		int sel_loop = (int)mSubtexture->get_loop();
		if (ImGui::Combo("Loop", &sel_loop, loops, IM_ARRAYSIZE(loops)))
		{
			mSubtexture->set_loop((engine::animation::loop_type)sel_loop);
			mModified = true;
		}

		engine::frect framerect = mSubtexture->get_root_frame();
		if (ImGui::DragFloat4("Dimensions", framerect.components, 1, 0, 0, "%.0f"))
		{ 
			mSubtexture->set_frame_rect(framerect.floor());
			mModified = true;
		}
	}

	if (mTexture)
	{
		ImGui::BeginChild("Atlas list", ImVec2(0, -25), true);
		for (auto& i : mTexture->get_texture_atlas().get_all())
		{
			if (ImGui::Selectable(i->get_name().c_str(), mSubtexture == i))
			{
				mSubtexture = i;
				mIs_playing = false;
				mCurrent_frame = mSubtexture->get_default_frame();

				// Center subtexture preview
				center_subtexture_preview();
			}
		}
		ImGui::EndChild();

		if (ImGui::Button("New"))
			ImGui::OpenPopup("###newentry");
		new_entry_popup();

		if (mSubtexture)
		{
			ImGui::SameLine();

			if (ImGui::Button("Rename"))
				ImGui::OpenPopup("Rename Atlas Entry");
			if (ImGui::BeginPopupModal("Rename Atlas Entry", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextColored({ 1, 1, 0.5, 1 }, "Warning: Tilemaps using this entry may break.");
				if (ImGui::InputText("Name", &mNew_entry_name, ImGuiInputTextFlags_EnterReturnsTrue)
					|| ImGui::Button("Rename", ImVec2(100, 25)))
				{
					mSubtexture->set_name(mNew_entry_name);
					mModified = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(100, 25)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Duplicate"))
			{
				auto dup = std::make_shared<engine::subtexture>(*mSubtexture);
				std::string name = mSubtexture->get_name();
				while (mTexture->get_entry(name += "_copy"));
				dup->set_name(name);
				mTexture->get_texture_atlas().add_entry(dup);
				mSubtexture = dup;
				mModified = true;
			}

			ImGui::SameLine();
			if (ImGui::Button("Delete"))
				ImGui::OpenPopup("###askfordeleteentry");
			if (ImGui::ConfirmPopup("Delete atlas entry?###askfordeleteentry", "Are you sure you want to delete this entry?") == 1)
			{
				mTexture->get_texture_atlas().remove_entry(mSubtexture);
				if (mTexture->get_texture_atlas().empty())
					mSubtexture.reset();
				else
					mSubtexture = mTexture->get_texture_atlas().get_all().front();
				mModified = true;
			}
		}
	}

	ImGui::EndChild();
	ImGui::End();
}

void atlas_imgui_editor::set_resource_manager(engine::resource_manager & pRes_mgr)
{
	mResource_manager = &pRes_mgr;
}

void atlas_imgui_editor::set_window_open_handler(bool* mBool)
{
	mWindow_open = mBool;
}

void atlas_imgui_editor::open_requested()
{
	auto resource = mResource_manager->get_resource<engine::texture>(engine::texture_restype, mReq_texture_name);
	mTexture = std::make_shared<engine::texture>();
	mTexture->set_texture_source(resource->get_texture_source());
	mTexture->set_atlas_source(resource->get_atlas_source());
	mTexture->set_name(resource->get_name());
	mTexture->load();

	mSubtexture.reset();
	mReq_texture_name.clear();
	mCurrent_frame = 0;
	mIs_playing = false;
	mModified = false;

	ImGui::UseRenderer(mFull_texture_renderdata);
	ImGui::SetRendererPan(mTexture->get_size() / 2);
	ImGui::EndRenderer();
}

bool atlas_imgui_editor::is_modified() const
{
	return mModified;
}

void atlas_imgui_editor::save()
{
	if (mTexture->get_texture_atlas().save(mTexture->get_atlas_source()))
		logger::info("Saved atlas to " + mTexture->get_atlas_source());
	else
		logger::error("Failed to save atlas to " + mTexture->get_atlas_source());
	mModified = false;
}

bool atlas_imgui_editor::is_confirming_save() const
{
	return ImGui::IsPopupOpen("###confirmsave");
}

void atlas_imgui_editor::confirm_save()
{
	if (mModified && !ImGui::IsPopupOpen("###confirmsave"))
		ImGui::OpenPopup("###confirmsave");
}

void atlas_imgui_editor::center_subtexture_preview()
{
	ImGui::UseRenderer(mSubtexture_renderdata);
	ImGui::SetRendererPan(mSubtexture->get_root_frame().get_size() / 2);
	ImGui::EndRenderer();
}

void atlas_imgui_editor::play_animation()
{
	mIs_playing = true;
	mCurrent_frame = mSubtexture->get_default_frame();
	mAnim_clock.restart();
}

void atlas_imgui_editor::stop_animation()
{
	mIs_playing = false;
}

void atlas_imgui_editor::update_animation()
{
	if (!mIs_playing)
		return;
	mCurrent_frame = mSubtexture->calc_frame_from_time(mAnim_clock.get_elapse().milliseconds());
	if (mSubtexture->get_loop() == engine::animation::loop_type::none
		&& mCurrent_frame == mSubtexture->get_frame_count() - 1)
		mIs_playing = false;
}

void atlas_imgui_editor::new_entry_popup()
{
	if (ImGui::BeginPopupModal("New entry###newentry", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Name", &mNew_entry_name);

		if (ImGui::Button("Create", ImVec2(100, 25)) && !mNew_entry_name.empty())
		{
			engine::subtexture::ptr nsb = std::make_shared<engine::subtexture>();
			nsb->set_name(mNew_entry_name);
			nsb->set_frame_rect({ engine::fvector(0, 0), mTexture->get_size() });
			mTexture->get_texture_atlas().add_entry(nsb);
			mSubtexture = nsb;
			mModified = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 25)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
