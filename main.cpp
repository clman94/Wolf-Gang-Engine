














// this is my sandbox. ignore plz


















#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <stack>
#include <functional>
#include <memory>
#include <cassert>
#include <type_traits>
#include <set>
#include <any>
#include <variant>
#include <typeindex>

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// OpenAL-Soft
#include <AL/al.h>
#include <AL/alc.h>

// STB
#include <stb/stb_vorbis.h>
#include <stb/stb_truetype.h>
#include <stb/stb_image.h>

// ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stl.h>

// WGE
#include <wge/core/object_node.hpp>
#include <wge/core/component.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/audio/ogg_vorbis_stream.hpp>
#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/matrix.hpp>
#include <wge/math/transformations.hpp>
#include <wge/util/clock.hpp>
#include <wge/util/ref.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/filesystem/input_stream.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world_component.hpp>
#include <wge/math/anchor.hpp>
#include <wge/math/rect.hpp>
#include <wge/filesystem/filesystem_interface.hpp>
#include <wge/core/context.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/scripting/script.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/graphics/sprite_component.hpp>
#include <wge/graphics/texture_asset_loader.hpp>
#include <wge/graphics/texture.hpp>
using namespace wge;

#include <Box2D/Box2D.h>

class flag_container
{
public:
	// Returns true if flag was successfully set
	bool set(const std::string& pName)
	{
		return mFlags.insert(pName).second;
	}

	// Returns true if flag was successfully unset
	bool unset(const std::string& pName)
	{
		return mFlags.erase(pName) > 0;
	}

	// Returns true if this container contains this value
	bool is_set(const std::string& pName) const
	{
		return mFlags.find(pName) != mFlags.end();
	}

	std::size_t matches(const flag_container& pOther)
	{
		
	}

	bool empty() const
	{
		return mFlags.empty();
	}

private:
	std::set<std::string> mFlags;
};

class collection
{
public:
	// Json will act as our map because it provides a lot of convenience
	// with is implicit convertions and easy serialization.
	using option_map = json;

	void set_name(const std::string& pName)
	{
		mName = pName;
	}

	json serialize() const
	{
		json result;
		result["name"] = mName;

		for (const auto& i : mMaps)
			result["maps"][i.first] = i.second;

		result["node"] = mRoot_node->serialize();
	}

	void deserialize(const json& pJson)
	{
		mName = pJson["name"];
		const json& maps = pJson["maps"];
		mMaps.clear();
		for (json::const_iterator i = maps.begin(); i != maps.end(); i++)
			mMaps[i.key()] = i.value();
		mRoot_node->deserialize(pJson["node"]);
	}

	option_map& operator[](const std::string& pString)
	{
		return mMaps[pString];
	}

	// Root node for this collection
	util::ref<core::object_node> get_node() const
	{
		return mRoot_node;
	}

private:
	util::ref<core::object_node> mRoot_node;
	std::map<std::string, option_map> mMaps;
	std::string mName;
};

void show_info(stb_vorbis *v)
{
	if (v) {
		stb_vorbis_info info = stb_vorbis_get_info(v);
		printf("%d channels, %d samples/sec\n", info.channels, info.sample_rate);
		printf("Predicted memory needed: %d (%d + %d)\n", info.setup_memory_required + info.temp_memory_required,
			info.setup_memory_required, info.temp_memory_required);
	}
}

class camera
{
public:
	void set_position(const math::vec2& pUnits)
	{
		mPosition = pUnits;
	}
	math::vec2 get_position() const
	{
		return mPosition;
	}
	
	void set_size(const math::vec2& pUnits)
	{
		mSize = pUnits;
	}
	math::vec2 get_size() const
	{
		return mSize;
	}

private:
	math::vec2 mPosition, mSize;
};



class physics_debug_component :
	public core::component
{
	WGE_COMPONENT("Physics Debug", 31234)
public:
	physics_debug_component(core::object_node* pNode) :
		component(pNode)
	{
		mWorld = nullptr;
		subscribe_to(pNode, "on_render", &physics_debug_component::on_render, this);
		subscribe_to(pNode, "on_physics_update_bodies", &physics_debug_component::on_physics_update_bodies, this);
		mDebug_draw.SetFlags(b2Draw::e_shapeBit);
	}

	void on_render(graphics::renderer* pRenderer)
	{
		if (mWorld)
		{
			mWorld->DrawDebugData();
			for (auto& i : mDebug_draw.batches)
				pRenderer->push_batch(i);
			mDebug_draw.batches.clear();
		}
	}

	void on_physics_update_bodies(physics::physics_world_component * pComponent)
	{
		mWorld = pComponent->get_world();
		mWorld->SetDebugDraw(&mDebug_draw);
	}

private:
	b2World* mWorld;
	struct debug_draw :
		b2Draw
	{
		std::list<graphics::render_batch_2d> batches;

		void create_polygon_batch(const b2Vec2* vertices, int32 vertexCount, const b2Color& pColor,
			graphics::render_batch_2d::primitive_type type, bool connect_end = true)
		{
			graphics::render_batch_2d batch;
			batch.type = type;
			batch.depth = -1;
			for (int i = 0; i < vertexCount; i++)
			{
				batch.indexes.push_back(batch.vertices.size());
				graphics::vertex_2d vert;
				vert.position = math::vec2(vertices[i].x, vertices[i].y);
				vert.color = graphics::color(pColor.r, pColor.g, pColor.b, pColor.a * 0.5f);
				batch.vertices.push_back(vert);
			}
			if (connect_end)
				batch.indexes.emplace_back(0);
			batches.push_back(batch);
		}

		void create_circle_batch(const b2Vec2& center, float32 radius, const b2Color& pColor,
			graphics::render_batch_2d::primitive_type type)
		{
			b2Vec2 verts[10];
			for (int i = 0; i < sizeof(verts); i++)
			{
				auto vert = math::vec2(center.x + radius, center.y).rotate(math::degrees(360.f / sizeof(verts)));
				verts[i] = b2Vec2(vert.x, vert.y);
			}
			create_polygon_batch(verts, sizeof(verts), pColor, type);
		}

		virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& pColor)
		{
			create_polygon_batch(vertices, vertexCount, pColor, graphics::render_batch_2d::type_linestrip);
		}

		virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& pColor)
		{
			create_polygon_batch(vertices, vertexCount, pColor, graphics::render_batch_2d::type_triangle_fan);
		}

		virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& pColor)
		{
			create_circle_batch(center, radius, pColor, graphics::render_batch_2d::type_linestrip);
		}

		virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& pColor)
		{
			create_circle_batch(center, radius, pColor, graphics::render_batch_2d::type_triangle_fan);
		}

		virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& pColor)
		{
			b2Vec2 verts[2];
			verts[0] = p1;
			verts[1] = p2;
			create_polygon_batch(verts, 2, pColor, graphics::render_batch_2d::type_linestrip, false);
		}

		virtual void DrawTransform(const b2Transform& xf)
		{
			// TODO
		}

		virtual void DrawPoint(const b2Vec2& p, float32 size, const b2Color& pColor)
		{
			create_circle_batch(p, size, pColor, graphics::render_batch_2d::type_triangle_fan);
		}

	} mDebug_draw;
};

// Failed attempt at using the stb_vorbis pushdata api.
// The pushdata api allows the user to give the stream a chunk of data and
// it spits out samples.
// While I do understand the advantage of this api, it is rather unstable
// when resyncing and requires the user to implement what is already implemented
// in the other api.
//
// This library should opt for a callback system like what was used in stb_image
// instead of hard coding in stdio. Or provide actually complete examples.
//
// When I get to implementing the .pack file format once again, stb_vorbis may be
// modified to use callbacks so it would be possible to compress the pack file.
//
void test_stream_al()
{
	/*ALCdevice* device = alcOpenDevice(NULL);
	ALCcontext* context = alcCreateContext(device, NULL);
	if (!alcMakeContextCurrent(context))
		std::cout << "could not use context\n";*/

	std::ifstream fstream("sonting.ogg", std::ios::binary | std::ios::ate);
	if (!fstream.good())
	{
		std::cout << "Failed to open file stream\n";
		return;
	}
	const std::size_t size = fstream.tellg();
	fstream.seekg(0);

	int error = 0;
	int consumed_memory = 0;
	stb_vorbis* vorb = nullptr;
	std::vector<std::uint8_t> data;
	data.resize(std::min((std::size_t)4096, size));
	do
	{
		fstream.seekg(0);
		fstream.read(reinterpret_cast<char*>(&data[0]), data.size());
		vorb = stb_vorbis_open_pushdata(&data[0], data.size(), &consumed_memory, &error, NULL);
		if (!vorb)
		{
			if (error == VORBIS_need_more_data && data.size() != size)
			{
				// Double the size of the array and retry opening the stream
				data.resize(std::min(data.size() * 2, size));
				std::cout << "Retrying with " << data.size() << " bytes\n";
			}
			else
			{
				// Error. Can't load at all.
				std::cout << "Error opening vorbis stream\n";
				return;
			}
		}
	} while (fstream.good() && !vorb);

	// Display some info about the stream or debugging
	stb_vorbis_info vorb_info = stb_vorbis_get_info(vorb);
	/*std::cout << "Channels: " << vorb_info.channels
		<< "\nSample Rate: " << vorb_info.sample_rate
		<< "\nFrame Size: " << vorb_info.max_frame_size
		<< "\nConsumed: " << consumed_memory 
		<< "\n";*/
	show_info(vorb);

	std::cout << "Sample length: " << stb_vorbis_stream_length_in_samples(vorb) << "\n";

	// Seek to end of the header
	fstream.seekg(consumed_memory);

	data.resize(1024*4);

	int channels = 0;
	float** samples = nullptr;
	int sample_count = 0;
	int tries = 0;
	do {
		fstream.read(reinterpret_cast<char*>(&data[0]), data.size());

		// FIXME: Only grabs about 1024 bytes max and needs to be called multiple times to actually fill a buffer.
		int bytes_used = stb_vorbis_decode_frame_pushdata(vorb, &data[0], data.size(), &channels, &samples, &sample_count);
		std::cout << "Samples read: " << sample_count
			<< "\nChannels read: " << channels
			<< "\nBytes used: " << bytes_used
			<< "\n\n";
		fstream.seekg((fstream.tellg().seekpos() - data.size()) + bytes_used - 1);
	} while (sample_count == 0 || tries++ < 10);
	/*int sample_array_size = channel_sample_count * channels * sizeof(short);

	// Create the buffer
	ALuint buffer;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, samples, sample_array_size, sample_rate);

	// Play it
	ALuint source = 0;
	alGenSources(1, &source);
	alSourcei(source, AL_BUFFER, buffer);
	alSourcePlay(source);*/

}

bool check_if_playing(GLuint pSource)
{
	ALint state = 0;
	alGetSourcei(pSource, AL_SOURCE_STATE, &state);
	return state == AL_PLAYING;
}

void test_stream_al2()
{
	audio::ogg_vorbis_stream vorb;
	vorb.open("sonting.ogg");

	ALCdevice* device = alcOpenDevice(NULL);
	ALCcontext* context = alcCreateContext(device, NULL);
	if (!alcMakeContextCurrent(context))
		std::cout << "could not use context\n";

	ALuint source = 0;
	alGenSources(1, &source);

	const int max_buffer_size = vorb.get_sample_rate();

	// We will use 3 buffers so there is plenty of time
	// for reading the file.
	ALuint buffer[3];
	alGenBuffers(3, &buffer[0]);

	// This buffer will hold the sample data.
	std::vector<short> samples;
	samples.resize(max_buffer_size);

	// Fill the buffers with some initial data
	for (int i = 0; i < 3; i++)
	{
		int amount = vorb.read(&samples[0], samples.size());
		if (amount == 0)
		{
			std::cout << "Reached limit of samples\n";

			// Don't queue any more buffers
			break;
		}
		alBufferData(buffer[i],
			vorb.get_channel_count() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
			&samples[0], amount * sizeof(short), vorb.get_sample_rate());
		alSourceQueueBuffers(source, 1, &buffer[i]);
	}

	bool looped = true;

	alSourcePlay(source);

	while (check_if_playing(source))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(22));

		// Get the amount of buffers that have been processed
		int processed_buffers = 0;
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed_buffers);

		while (processed_buffers--)
		{
			// Remove the buffer from the queue to be filled again
			ALuint buffer = 0;
			alSourceUnqueueBuffers(source, 1, &buffer);

			// Read the new data
			int amount = vorb.read(&samples[0], samples.size());
			if (amount == 0)
			{
				if (looped)
				{
					// Seek the stream to the beginning
					// for a seamless loop.
					std::cout << "Resetting stream for loop\n";
					vorb.seek_beginning();

					// Read some new data
					vorb.read(&samples[0], samples.size());
				}
				else
				{
					// Don't add the buffer back again if we ran out of data
					std::cout << "Nothing to fill buffer\n";
					continue;
				}
			}
			std::cout << "Read some new ones " << amount << "\n";

			// Fill the buffer
			alBufferData(buffer,
				vorb.get_channel_count() == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
				&samples[0], amount * sizeof(short), vorb.get_sample_rate());

			// Queue it to be processed again
			alSourceQueueBuffers(source, 1, &buffer);
		}
	}
	std::cout << "stopped";
}

ALuint test_al()
{
	ALCdevice* device = alcOpenDevice(NULL);
	ALCcontext* context = alcCreateContext(device, NULL);
	if (!alcMakeContextCurrent(context))
		std::cout << "could not use context\n";

	// Load the sound
	int channels = 0;
	int sample_rate = 0;
	short* samples = nullptr;

	// This actually returns the amount of samples in a single channel
	// so we will need to multiply this value by the number of channels to get
	// the total amount of samples in this file
	int channel_sample_count = stb_vorbis_decode_filename("sonting.ogg", &channels, &sample_rate, &samples);

	int sample_array_size = channel_sample_count * channels * sizeof(short);

	// Create the buffer
	ALuint buffer;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, samples, sample_array_size, sample_rate);

	free(samples);

	// Play it
	ALuint source = 0;
	alGenSources(1, &source);
	alSourcei(source, AL_BUFFER, buffer);
	alSourcePlay(source);
	
	return source;
	// Cleanup
	/*alDeleteSources(1, &source);
	alDeleteBuffers(1, &buffer);
	device = alcGetContextsDevice(context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);*/
}


namespace ImGui
{

// Draws a framebuffer as a regular image
void Image(const graphics::framebuffer& mFramebuffer, const ImVec2& pSize = ImVec2(0, 0))
{
	ImGui::Image((void*)mFramebuffer.get_gl_texture(), pSize,
		ImVec2(0, 1), ImVec2(1, 0)); // Y-axis needs to be flipped
}

}

struct pieclass
{
	int x;
	void func(int i)
	{
	}
};

void thing()
{
	using namespace wge::scripting;

	script myscript;

	// Register a lambda/anonymous function. AngelScript can't do this
	// with its own api.
	// Defines a global function: "void print(string)"
	myscript.global("print",
		function([](std::string pStr, int)
		{
			std::cout << "Print: " << pStr << "\n";
		}));

	// Get the function
	auto print_func = myscript.get_function<void(std::string, int)>("print");

	// Call it directly
	print_func("yey", 34);

	// Start a new module
	myscript.module();
	// Add a file
	myscript.file("./astest.as");
	// Compile it
	myscript.build();


	// Register the pie class
	//myscript.value<pieclass>("pie");
	//myscript.object("pie", "x", member(&pieclass::x));
	//myscript.object("pie", "func", function(&pieclass::func));

	// Register a global variable
	//pieclass mypie;
	//myscript.global("mypie", std::ref(mypie));

	// Register a lambda function
}

// The goal behind this test was to implement coroutine functionality
// with more convenience. The previous version of the engine used something
// called create_thread(coroutine@, dictionary), in which you provide a function that can only take
// a dictionary type for its one and only parameter.
//
// From this mess:
// funcdef void func(dictionary); // Create the function definition
// start_coroutine(func(myfuncimpl), dictionary = {{"arg1", 23}}); // Create the dictionary and pass it to the function
//
// To this:
// start_coroutine("myfuncimplglob", 23) // Global function
// start_coroutine(myclass, "myfuncimpl", 23) // Class function
//
// This would greatly simplify the creation of coroutines and give it practicality.
// You can pretty much start a coroutine with almost any function and it
// doesn't limit you to using the dictionaries to pass data to it.
//
// To create a coroutine, you call the coroutine::start([ref,] string[, params, ...]) function.
// A coroutine gets access to these functions:
//   bool coroutine::yield();
//   bool coroutine::wait(float pSeconds);
//
// To achieve this, the variable parameter feature angelscript provides seemed promising.
// Just declare it as so:
// void start_coroutine(const string&in declar, ?&in) // Global function
// void start_coroutine(const ref&in class_ref, const string&in declar, ?&in) // Method
//
// The only caveat is that you can't use &inout references so you'll have to settle with passing handles.
// Note: When you want to pass an argument as a handle, remember to put a @ in front it. e.g. "coroutine::start("void func(clazz@)", @myclazz)"
//   If you forget to, the object will be copied.
//

// This copies an argument to another parameter in angelscript. May still need some work as it hasn't been fully tested in all situations.
void copy_argument(AngelScript::asIScriptEngine* engine, void* dest, int dest_typeid, void* src, int src_typeid)
{
	using namespace AngelScript;
	if (dest_typeid & asTYPEID_OBJHANDLE)
	{
		// Dereference the handle if the input is also a reference
		if (src_typeid & asTYPEID_OBJHANDLE)
			std::memcpy(dest, *(void**)src, sizeof(void*));
		else
			std::memcpy(dest, src, sizeof(void*));

		// Add a reference to the object so it won't be destroyed.
		engine->AddRefScriptObject(*(void**)dest, engine->GetTypeInfoById(dest_typeid));
	}
	else if (dest_typeid & asTYPEID_MASK_OBJECT)
	{
		// Copy the pointer to the object
		std::memcpy(dest, src, sizeof(void*));
	}
	else
	{
		// Copy the primitive value
		int size = engine->GetSizeOfPrimitiveType(dest_typeid);
		std::memcpy(dest, *(void**)src, size);
	}
}

void test_as_varfunc()
{
	using namespace AngelScript;

	asIScriptEngine* engine = asCreateScriptEngine();

	// Register a message callback so we can see whats happening.
	// This is just copied from the angelscript documentation because lazy.
	void (*MessageCallback)(const asSMessageInfo *msg, void *param) =
		[](const asSMessageInfo *msg, void *param)
	{
		const char *type = "ERR ";
		if (msg->type == asMSGTYPE_WARNING)
			type = "WARN";
		else if (msg->type == asMSGTYPE_INFORMATION)
			type = "INFO";
		std::printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
	};
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

	// Register some basic utilities
	RegisterScriptHandle(engine);
	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);

	// Register a print function so we can debug things
	void(*print)(asIScriptGeneric*) = [](asIScriptGeneric* pGen)
	{
		std::string* msg = reinterpret_cast<std::string*>(pGen->GetArgObject(0));
		std::cout << *msg << "\n";
	};
	engine->RegisterGlobalFunction("void print(const string&in)", asFUNCTION(print), asCALL_GENERIC);

	void(*varfunc)(asIScriptGeneric*) = [](asIScriptGeneric* pGen)
	{
		asIScriptEngine* engine = pGen->GetEngine();
		CScriptHandle* handle = reinterpret_cast<CScriptHandle*>(pGen->GetArgObject(0));
		std::string* declar = reinterpret_cast<std::string*>(pGen->GetArgObject(1));

		// Query the function declaration
		asIScriptFunction* type = handle->GetType()->GetMethodByDecl(declar->c_str());
		if (!type)
		{
			std::cout << "Could not func method \"" << *declar << " in class \"" << handle->GetType()->GetName() << "\"\n";
			return;
		}

		// Make sure the parameter and the argument count are the same
		if (type->GetParamCount() != pGen->GetArgCount() - 2)
		{
			std::cout << "Invalid amount of parameters\n";
			return;
		}

		// Setup the context
		asIScriptContext* ctx = engine->CreateContext();
		int err = ctx->Prepare(type);

		// TODO: Make a version that can call global functions
		ctx->SetObject(handle->GetRef());

		if (err == 0)
		{
			// Copy the arguments
			for (int i = 0; i < pGen->GetArgCount() - 2; i++)
			{
				// Get destination information
				void* dest = ctx->GetAddressOfArg(i);
				int dest_typeid = 0;
				type->GetParam(i, &dest_typeid);

				// Get source information
				void* src = pGen->GetAddressOfArg(i + 2);
				int src_typeid = pGen->GetArgTypeId(i + 2);

				// Copy the source to the destination
				copy_argument(engine, dest, dest_typeid, src, src_typeid);
			}

			// Execute. In this case, it expects a terminating/non-suspending function.
			// Modifications may need to be made to support coroutines e.g. a context manager. 
			ctx->Execute();
			ctx->Unprepare();
		}
		engine->ReturnContext(ctx);
	};

	// This can handle as many variable parameters as needed
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in)", asFUNCTION(varfunc), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in, ?&in)", asFUNCTION(varfunc), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in, ?&in, ?&in)", asFUNCTION(varfunc), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in, ?&in, ?&in, ?&in)", asFUNCTION(varfunc), asCALL_GENERIC);

	// Build the script
	CScriptBuilder builder;
	builder.StartNewModule(engine, "firstmod");
	builder.AddSectionFromFile("astest.as");
	builder.BuildModule();

	// Execute it
	asIScriptContext* ctx = engine->CreateContext();
	ctx->Prepare(engine->GetModule("firstmod")->GetFunctionByDecl("void start()"));
	ctx->Execute();
	ctx->Unprepare();
	engine->ReturnContext(ctx);

	engine->ShutDownAndRelease();
}

// This class stores a list of inspectors to be used for
// each type of component.
class editor_component_inspector
{
public:
	// Assign an inspector for a component
	void add_inspector(int pComponent_id, std::function<void(core::component*)> pFunc)
	{
		mInspector_guis[pComponent_id] = pFunc;
	}
	
	// Show the inspector's gui for this component
	void on_gui(core::component* pComponent)
	{
		if (auto func = mInspector_guis[pComponent->get_component_id()])
			func(pComponent);
	}

private:
	std::map<int, std::function<void(core::component*)>> mInspector_guis;
};

bool collapsing_arrow(const char* pStr_id, bool* pOpen = nullptr , bool pDefault_open = false)
{
	ImGui::PushID(pStr_id);

	// Use internal instead
	if (!pOpen)
		pOpen = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("IsOpen"), pDefault_open);;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
	if (ImGui::ArrowButton("Arrow", *pOpen ? ImGuiDir_Down : ImGuiDir_Right))
		*pOpen = !*pOpen; // Toggle open flag
	ImGui::PopStyleColor(3);
	ImGui::PopID();
	return *pOpen;
}

void show_node_tree(util::ref<core::object_node> pNode, editor_component_inspector& pInspector, util::ref<core::object_node>& pSelected)
{
	ImGui::PushID(pNode.get());

	bool* open = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("IsOpen"));

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	// Don't show the arrow if there are no children nodes
	if (pNode->get_child_count() > 0)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		collapsing_arrow("NodeUnfold", open);
		ImGui::PopStyleVar();
		ImGui::SameLine();
	}

	if (ImGui::Selectable(pNode->get_name().c_str(), pSelected == pNode))
		pSelected = pNode;
	if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
		*open = !*open; // Toggle open flag
	if (ImGui::BeginDragDropSource())
	{
		core::object_node* ptr = pNode.get();
		ImGui::SetDragDropPayload("MoveNodeInTree", &ptr, sizeof(void*));

		ImGui::Text(pNode->get_name().c_str());

		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MoveNodeInTree"))
		{
			util::ref<core::object_node> node = *static_cast<core::object_node**>(payload->Data);
			if (!pNode->is_child_of(node)) // Do not move parent into its own child!
				pNode->add_child(node);
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::InvisibleButton("__DragBetween", ImVec2(-1, 3));
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MoveNodeInTree"))
		{
			util::ref<core::object_node> node = *static_cast<core::object_node**>(payload->Data);
			if (!pNode->is_child_of(node)) // Do not move parent into its own child!
			{
				if (pNode->get_child_count() && *open)
					pNode->add_child(node, 0);
				else if (auto parent = pNode->get_parent())
					parent->add_child(node, parent->get_child_index(pNode));
			}
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);

	if (*open)
	{
		ImGui::TreePush();
		// Show the children nodes and their components
		for (std::size_t i = 0; i < pNode->get_child_count(); i++)
			show_node_tree(pNode->get_child(i), pInspector, pSelected);
		ImGui::TreePop();
		if (pNode->get_child_count() > 0 && *open)
		{
			ImGui::InvisibleButton("__DragAfterChildrenInParent", ImVec2(-1, 3));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MoveNodeInTree"))
				{
					util::ref<core::object_node> node = *static_cast<core::object_node**>(payload->Data);
					if (!pNode->is_child_of(node)) // Do not move parent into its own child!
						if (auto parent = pNode->get_parent())
							parent->add_child(node, parent->get_child_index(pNode) + 1);
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
		}
	}

	ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

	ImGui::PopID();
}

// A theme I made up for imgui. I wanted it to not be
// distracting and have a pleasing subtlety to it.
void use_default_style()
{
	ImGuiIO& io = ImGui::GetIO();
	io.FontDefault = io.Fonts->AddFontFromFileTTF("editor/Roboto-Medium.ttf", 30);
	io.FontDefault->Scale = 0.5f;

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 2;
	style.ChildRounding = 5;
	style.FrameRounding = 2;
	style.FrameBorderSize = 1;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 0.97f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.31f, 0.31f, 0.31f, 0.47f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.18f, 0.71f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.23f, 0.23f, 0.23f, 0.27f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.36f, 0.36f, 0.36f, 0.70f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.09f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.55f, 0.55f, 0.55f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.77f, 0.85f, 0.93f, 0.77f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

class test_component :
	public core::component
{
	WGE_COMPONENT("Test", 1230);
public:
	test_component(core::object_node* pNode) :
		core::component(pNode)
	{
		subscribe_to(pNode, "on_update", std::function([]()
		{
			std::cout << "update";
		}));
	}
};

void GLAPIENTRY MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

namespace ImGui
{

// A cool little idea for later
void BeginChildWithHeader(const char* pTitle, const ImVec2& pSize = ImVec2(0, 0))
{
	ImVec2 backup_padding = ImGui::GetStyle().WindowPadding;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::BeginChild(pTitle, pSize, true);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, backup_padding);


	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Pop the clip rect so we can draw the header correctly
	ImVec4 cliprect = dl->_ClipRectStack.back();
	dl->PopClipRect();

	// Draw the background
	dl->AddRectFilled(ImGui::GetWindowPos(),
		ImVec2(ImGui::GetWindowWidth() + ImGui::GetWindowPos().x, 25.f + ImGui::GetWindowPos().y),
		ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]),
		ImGui::GetStyle().ChildRounding, ImDrawCornerFlags_Top);

	// Draw the title
	dl->AddText(ImVec2(ImGui::GetWindowPos().x + 10, ImGui::GetWindowPos().y + 4),
		ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]),
		pTitle);

	// Put the clip rect back
	dl->PushClipRect(ImVec2(cliprect.x, cliprect.y), ImVec2(cliprect.z, cliprect.w));

	ImGui::Dummy(ImVec2(0, 25));

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
	ImGui::BeginChild(pTitle);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

void EndChildWithHeader()
{
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

}


struct editor_state
{
	math::vec2 offset;
	math::vec2 scale;
	math::vec2 mouse_position; // In pixels
	math::vec2 mouse_editor_position; // Scaled and offsetted
	ImGuiID active_dragger_id{ 0 };

	math::vec2 calc_absolute_position(math::vec2 pPos) const
	{
		return pPos * scale + offset;
	}
};

std::vector<editor_state> gEditor_states;
editor_state* gCurrent_editor_state = nullptr;

void begin_editor(const char* pStr_id, math::vec2 pCursor_offset, math::vec2 pScale)
{
	ImGui::PushID(pStr_id);

	// First time
	if (ImGui::GetStateStorage()->GetInt(ImGui::GetID("StateIndex"), -1) == -1)
	{
		ImGui::GetStateStorage()->SetInt(ImGui::GetID("StateIndex"), gEditor_states.size());
		editor_state state;
		gEditor_states.emplace_back();
	}
	
	// Update the state
	int* index = ImGui::GetStateStorage()->GetIntRef(ImGui::GetID("StateIndex"), gEditor_states.size());
	gCurrent_editor_state = &gEditor_states[*index];
	gCurrent_editor_state->offset = pCursor_offset;
	gCurrent_editor_state->scale = pScale;
	gCurrent_editor_state->mouse_position = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
	gCurrent_editor_state->mouse_editor_position = (gCurrent_editor_state->mouse_position - pCursor_offset) / pScale;
}

void end_editor()
{
	assert(gCurrent_editor_state);
	gCurrent_editor_state = nullptr;
	ImGui::PopID();
}

bool drag_behavior(ImGuiID pID, bool pHovered)
{
	assert(gCurrent_editor_state);
	bool dragging = gCurrent_editor_state->active_dragger_id == pID;
	if (pHovered && ImGui::IsItemClicked(0) && gCurrent_editor_state->active_dragger_id == 0)
	{
		gCurrent_editor_state->active_dragger_id = pID; // Start drag
		dragging = true;
	}
	else if (!ImGui::IsMouseDown(0) && dragging)
	{
		gCurrent_editor_state->active_dragger_id = 0; // End drag
		dragging = true; // Return true for one more frame after the mouse is released
		                 // so the user can handle mouse-released events.
	}
	return dragging;
}

bool drag_behavior(ImGuiID pID, bool pHovered, float* pX, float* pY)
{
	bool dragging = drag_behavior(pID, pHovered);
	if (dragging)
	{
		if (pX)
			*pX += ImGui::GetIO().MouseDelta.x / gCurrent_editor_state->scale.x;
		if (pY)
			*pY += ImGui::GetIO().MouseDelta.y / gCurrent_editor_state->scale.y;
	}
	return dragging;
}

bool drag_behavior(ImGuiID pID, bool pHovered, math::vec2* pVec)
{
	return drag_behavior(pID, pHovered, &pVec->x, &pVec->y);
}

bool drag_circle(const char* pStr_id, math::vec2 pPos, math::vec2* pDelta, float pRadius)
{
	assert(gCurrent_editor_state);
	const ImGuiID id = ImGui::GetID(pStr_id);
	const math::vec2 center = gCurrent_editor_state->calc_absolute_position(pPos);
	const bool hovered = gCurrent_editor_state->mouse_position.distance(center) <= pRadius;
	const bool dragging = drag_behavior(id, hovered, pDelta);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddCircle({ center.x, center.y }, pRadius, ImGui::GetColorU32({ 1, 1, 0, 0.5f }));

	return dragging;
}

bool drag_circle(const char* pStr_id, math::vec2* pDelta, float pRadius)
{
	return drag_circle(pStr_id, *pDelta, pDelta, pRadius);
}

bool drag_rect(const char* pStr_id, math::rect* pRect)
{
	const ImGuiID id = ImGui::GetID(pStr_id);
	const bool hovered = pRect->intersects(gCurrent_editor_state->mouse_editor_position);
	const bool dragging = drag_behavior(id, hovered, &pRect->position);

	const math::vec2 min = gCurrent_editor_state->calc_absolute_position(pRect->position);
	const math::vec2 max = gCurrent_editor_state->calc_absolute_position(pRect->position + pRect->size);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRect({ min.x, min.y }, { max.x, max.y }, ImGui::GetColorU32({ 1, 1, 0, 0.5f }));

	return dragging;
}

bool drag_resizable_rect(const char* pStr_id, math::rect* pRect)
{
	bool dragging = false;

	ImGui::PushID(pStr_id);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->ChannelsSplit(2);
	dl->ChannelsSetCurrent(1);

	math::vec2 topleft;
	if (drag_circle("TopLeft", pRect->position, &topleft, 6))
	{
		dragging = true;
		pRect->position += topleft;
		pRect->size -= topleft;
	}

	dragging |= drag_circle("BottomRight", pRect->get_corner(2), &pRect->size, 6);

	math::vec2 topright;
	if (drag_circle("TopRight", pRect->get_corner(1),  &topright, 6))
	{
		dragging = true;
		pRect->y += topright.y;
		pRect->width += topright.x;
		pRect->height -= topright.y;
	}

	math::vec2 bottomleft;
	if (drag_circle("BottomLeft", pRect->get_corner(3), &bottomleft, 6))
	{
		dragging = true;
		pRect->x += bottomleft.x;
		pRect->width -= bottomleft.x;
		pRect->height += bottomleft.y;
	}

	math::vec2 top;
	if (drag_circle("Top", pRect->position + math::vec2(pRect->width / 2, 0), &top, 6))
	{
		dragging = true;
		pRect->y += top.y;
		pRect->height -= top.y;
	}

	math::vec2 bottom;
	if (drag_circle("Bottom", pRect->position + math::vec2(pRect->width / 2, pRect->height), &bottom, 6))
	{
		dragging = true;
		pRect->height += bottom.y;
	}

	math::vec2 left;
	if (drag_circle("Left", pRect->position + math::vec2(0, pRect->height / 2), &left, 6))
	{
		dragging = true;
		pRect->x += left.x;
		pRect->width -= left.x;
	}

	math::vec2 right;
	if (drag_circle("Right", pRect->position + math::vec2(pRect->width, pRect->height / 2), &right, 6))
	{
		dragging = true;
		pRect->width += right.x;
	}

	dl->ChannelsSetCurrent(0);

	dragging |= drag_rect("RectMove", pRect);

	dl->ChannelsMerge();

	ImGui::PopID();

	return dragging;
}

int main()
{

	thing();
	glfwInit();

	// OpenGL 3.3 minimum
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Make Mac happy
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // No old OpenGL

	// Create our window and initialize the opengl context
	GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
	glfwMakeContextCurrent(window);
	glewInit();

	// Match the monitors refresh rate (VSync)
	glfwSwapInterval(1);

	// Print the paths of all the files that were dropped into the window
	// to test its functionality.
	glfwSetDropCallback(window, [](GLFWwindow* pWindow, int pCount, const char** pPaths)
	{
		for (int i = 0; i < pCount; i++)
			std::cout << pPaths[i] << "\n";
	});

	// Setup ImGui
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 150");

	use_default_style();

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::thread al_test_thread;

	editor_component_inspector inspector_guis;

	// Inspector for transform_component
	inspector_guis.add_inspector(core::transform_component::COMPONENT_ID,
		[](core::component* pComponent)
	{
		auto reset_context_menu = [](const char * pId)->bool
		{
			bool clicked = false;
			if (ImGui::BeginPopupContextItem(pId))
			{
				if (ImGui::Button("Reset"))
				{
					clicked = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			return clicked;
		};

		auto transform = dynamic_cast<core::transform_component*>(pComponent);
		math::vec2 position = transform->get_position();
		if (ImGui::DragFloat2("Position", position.components))
			transform->set_position(position);
		if (reset_context_menu("posreset"))
			transform->set_position(math::vec2(0, 0));

		float rotation = math::degrees(transform->get_rotation());
		if (ImGui::DragFloat("Rotation", &rotation, 1, 0, 0, "%.3f degrees"))
			transform->set_rotaton(math::degrees(rotation));
		if (reset_context_menu("rotreset"))
			transform->set_rotaton(0);

		math::vec2 scale = transform->get_scale();
		if (ImGui::DragFloat2("Scale", scale.components, 0.01f))
			transform->set_scale(scale);
		if (reset_context_menu("scalereset"))
			transform->set_scale(math::vec2(0, 0));
	});

	// Inspector for sprite_component
	inspector_guis.add_inspector(graphics::sprite_component::COMPONENT_ID,
		[](core::component* pComponent)
	{
		auto sprite = dynamic_cast<graphics::sprite_component*>(pComponent);
		math::vec2 offset = sprite->get_offset();
		if (ImGui::DragFloat2("Offset", offset.components))
			sprite->set_offset(offset);

		graphics::texture::ptr tex = sprite->get_texture();
		std::string inputtext = tex ? tex->get_path().string().c_str() : "None";
		ImGui::InputText("Texture", &inputtext, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("textureAsset"))
			{
				core::asset_uid id = *(core::asset_uid*)payload->Data;
				sprite->set_texture(id);
			}
			ImGui::EndDragDropTarget();
		}
	});

	// Inspector for physics_world_component
	inspector_guis.add_inspector(physics::physics_world_component::COMPONENT_ID,
		[](core::component* pComponent)
	{
		auto physicsworld = dynamic_cast<physics::physics_world_component*>(pComponent);
		math::vec2 gravity = physicsworld->get_gravity();
		if (ImGui::DragFloat2("Gravity", gravity.components))
			physicsworld->set_gravity(gravity);
	});

	// Inspector for physics_box_collider
	inspector_guis.add_inspector(physics::box_collider_component::COMPONENT_ID,
		[](core::component* pComponent)
	{
		auto collider = dynamic_cast<physics::box_collider_component*>(pComponent);

		math::vec2 offset = collider->get_offset();
		if (ImGui::DragFloat2("Offset", offset.components))
			collider->set_offset(offset);

		math::vec2 size = collider->get_size();
		if (ImGui::DragFloat2("Size", size.components))
			collider->set_size(size);

		float rotation = math::degrees(collider->get_rotation());
		if (ImGui::DragFloat("Rotation", &rotation))
			collider->set_rotation(math::degrees(rotation));
	});

	core::context mycontext;
	core::component_factory& factory = mycontext.get_component_factory();
	factory.add<core::transform_component>();
	factory.add<physics::physics_world_component>();
	factory.add<physics::physics_component>();
	factory.add<physics::box_collider_component>();
	factory.add<graphics::sprite_component>();
	factory.add<physics_debug_component>();

	graphics::texture_asset_loader mytexture_loader;
	core::asset_manager myassetmanager;
	myassetmanager.add_loader("texture", &mytexture_loader);
	myassetmanager.set_root_directory(".");
	//myassetmanager.import_asset("./mytex.png");
	myassetmanager.load_assets();

	mycontext.add_system(&myassetmanager);

	auto root_node = core::object_node::create(&mycontext);
	root_node->set_name("Game");
	root_node->add_component<physics::physics_world_component>();
	root_node->add_component<physics_debug_component>();
	{
		auto node1 = root_node->create_child();
		node1->set_name("Scene node 1");
		node1->add_component<core::transform_component>();
		auto sprite = node1->add_component<graphics::sprite_component>();
		sprite->set_texture("mytex.png");
		node1->add_component<physics::physics_component>();

		{
			auto node2 = node1->create_child();
			node2->set_name("Scene node 2");
			node2->add_component<core::transform_component>();
			node2->add_component<graphics::sprite_component>();
			node2->add_component<physics::box_collider_component>();
		}

		{
			auto node3 = root_node->create_child();
			node3->set_name("Scene node 3");
			auto transform = node3->add_component<core::transform_component>();
			transform->set_position(math::vec2(0, 100));
			node3->add_component<graphics::sprite_component>();
			auto physics = node3->add_component<physics::physics_component>();
			physics->set_type(physics::physics_component::type_static);
			auto collider = node3->add_component<physics::box_collider_component>();
			collider->set_size(math::vec2(200, 10));
		}
	}

	graphics::framebuffer myframebuffer;
	myframebuffer.create(200, 200);
	graphics::renderer myrenderer;
	myrenderer.initialize();
	myrenderer.set_framebuffer(&myframebuffer);
	myrenderer.set_pixel_size(0.01f);

	bool updates_enabled = false;

	util::ref<core::object_node> selected_object;

	util::clock clock;
	while (!glfwWindowShouldClose(window))
	{
		// Calculate delta
		float delta = clock.restart();

		// Get window events
		glfwPollEvents();

		if (updates_enabled)
		{
			// Send update events
			root_node->send_down("on_preupdate", delta);
			root_node->send_down("on_update", delta);
		}

		// Start a new frame for ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Begin("Game"))
		{
			ImGui::Text("FPS: %f", 1.f / delta);

			if (ImGui::Button("Serialize"))
			{
				json j = root_node->serialize();
				std::ofstream{ "./serialization_data.json" } << j.dump(2);
			}
			ImGui::SameLine();
			if (ImGui::Button("Deserialize"))
			{
				json deserial;
				std::ifstream{ "./serialization_data.json" } >> deserial;
				root_node->remove_children();
				root_node->remove_components();
				root_node->deserialize(deserial);
				selected_object.reset();
			}
			ImGui::Checkbox("Run", &updates_enabled);
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, ImGui::GetStyle().WindowPadding.y));
		if (ImGui::Begin("Objects", NULL, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Add"))
				{
					if (ImGui::MenuItem("Object 2D"))
					{
						auto node = core::object_node::create(&mycontext);
						node->set_name("New 2D Object");
						node->add_component<core::transform_component>();
						if (selected_object)
							selected_object->add_child(node);
						else
							root_node->add_child(node);
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			show_node_tree(root_node, inspector_guis, selected_object);
		}
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Component Inspector");
		if (selected_object)
		{
			std::string name = selected_object->get_name();
			if (ImGui::InputText("Name", &name))
				selected_object->set_name(name);

			for (std::size_t i = 0; i < selected_object->get_component_count(); i++)
			{
				// If set to true, the component will be deleted at the end of this loop
				bool delete_component = false;

				core::component* comp = selected_object->get_component_index(i);
				ImGui::PushID(comp);

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
				ImGui::BeginChild(ImGui::GetID("Actions"),
					ImVec2(0, (ImGui::GetStyle().WindowPadding.y * 2
						+ ImGui::GetStyle().FramePadding.y * 2
						+ ImGui::GetFontSize()) * 2), true);

				bool open = collapsing_arrow("CollapsingArrow");

				ImGui::SameLine();
				{
					ImGui::PushItemWidth(150);
					std::string name = comp->get_name();
					if (ImGui::InputText("##NameInput", &name))
						comp->set_name(name);
					ImGui::PopItemWidth();
				}
				
				ImGui::SameLine();
				ImGui::Text(comp->get_component_name().c_str());

				ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionWidth()
					- (ImGui::CalcTextSize("Delete ").x
						+ ImGui::GetStyle().WindowPadding.x * 2
						+ ImGui::GetStyle().FramePadding.x * 2), 1));
				ImGui::SameLine();;
				delete_component = ImGui::Button("Delete");

				ImGui::EndChild();
				ImGui::PopStyleVar();

				if (open)
					inspector_guis.on_gui(comp);

				ImGui::PopID();
				if (delete_component)
					selected_object->remove_component(i--);
			}

			ImGui::Separator();
			if (ImGui::BeginCombo("###Add Component", "Add Component"))
			{
				if (ImGui::Selectable("Transform 2D"))
					selected_object->add_component<core::transform_component>();
				if (ImGui::Selectable("Physics World"))
					selected_object->add_component<physics::physics_world_component>();
				if (ImGui::Selectable("Physics"))
					selected_object->add_component<physics::physics_component>();
				if (ImGui::Selectable("Box Collider"))
					selected_object->add_component<physics::box_collider_component>();
				if (ImGui::Selectable("Sprite"))
					selected_object->add_component<graphics::sprite_component>();
				ImGui::EndCombo();
			}
		}
		ImGui::End();

		if (ImGui::Begin("Asset Manager"))
		{
			ImGui::Columns(3, "_AssetColumns");
			ImGui::SetColumnWidth(0, 100);

			ImGui::TextUnformatted("Type:");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Path:");
			ImGui::NextColumn();
			ImGui::TextUnformatted("UID:");
			ImGui::NextColumn();

			for (auto& i : myassetmanager.get_asset_list())
			{
				ImGui::PushID(i->get_id());
				ImGui::Selectable(i->get_type().c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
				if (ImGui::BeginDragDropSource())
				{
					core::asset_uid id = i->get_id();
					ImGui::SetDragDropPayload((i->get_type() + "Asset").c_str(), &id, sizeof(core::asset_uid));
					ImGui::Text("Asset");
					ImGui::EndDragDropSource();
				}
				ImGui::NextColumn();
				ImGui::TextUnformatted(i->get_path().string().c_str());
				ImGui::NextColumn();
				ImGui::TextUnformatted(std::to_string(i->get_id()).c_str());
				ImGui::NextColumn();
				ImGui::PopID();
			}
			ImGui::Columns();
		}
		ImGui::End();

		ImGui::Begin("Viewport");

		float width = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x*2;
		float height = ImGui::GetWindowHeight() - ImGui::GetCursorPos().y - ImGui::GetStyle().WindowPadding.y;

		if (myframebuffer.get_width() != width
			|| myframebuffer.get_height() != height)
			myframebuffer.resize(width, height);

		ImGui::Image(myframebuffer, ImVec2(width, height));

		ImGui::End();

		ImGui::Begin("Hello, world!");

		if (ImGui::CollapsingHeader("Style Editor"))
			ImGui::ShowStyleEditor();

		if (ImGui::CollapsingHeader("AngelScript Test"))
		{
			if (ImGui::Button("Test"))
				test_as_varfunc();
		}

		if (ImGui::CollapsingHeader("AL Test"))
		{
			if (ImGui::Button("Test"))
				al_test_thread = std::thread(&test_stream_al2);
		}
		ImGui::End();

		if (ImGui::Begin("Test Bars", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
		{
			float* zoom = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("_Zoom"), 0);

			const ImVec2 last_cursor = ImGui::GetCursorPos();
			ImGui::BeginGroup();

			const float scale = std::powf(2, (*zoom));
			const ImVec2 image_size((float)myframebuffer.get_width()*scale, (float)myframebuffer.get_height()*scale);

			// Top and left padding
			ImGui::Dummy(ImVec2(image_size.x + ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
			ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, image_size.y));
			ImGui::SameLine();

			const ImVec2 image_position = ImGui::GetCursorScreenPos();
			ImGui::Image(myframebuffer, image_size);

			// Right and bottom padding
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, image_size.y));
			ImGui::Dummy(ImVec2(image_size.x + ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
			ImGui::EndGroup();

			// Draw grid
			if (*zoom > 2)
			{
				// Horizontal lines
				ImDrawList* dl = ImGui::GetWindowDrawList();
				for (float i = 0; i < myframebuffer.get_width(); i++)
				{
					const float x = image_position.x + i * scale;
					if (x > ImGui::GetWindowPos().x && x < ImGui::GetWindowPos().x + ImGui::GetWindowWidth())
						dl->AddLine(ImVec2(x, image_position.y),
							ImVec2(x, image_position.y + image_size.y),
							ImGui::GetColorU32(ImVec4(0, 1, 1, 0.2f)));
				}

				// Vertical lines
				for (float i = 0; i < myframebuffer.get_height(); i++)
				{
					const float y = image_position.y + i * scale;
					if (y > ImGui::GetWindowPos().y && y < ImGui::GetWindowPos().y + ImGui::GetWindowHeight())
						dl->AddLine(ImVec2(image_position.x, y),
							ImVec2(image_position.x + image_size.x, y),
							ImGui::GetColorU32(ImVec4(0, 1, 1, 0.2f)));
				}
			}

			// Overlap with an invisible button to recieve input
			ImGui::SetCursorPos(last_cursor);
			ImGui::InvisibleButton("_Input", ImVec2(image_size.x + ImGui::GetWindowWidth(), image_size.y + ImGui::GetWindowHeight()));

			begin_editor("_SomeEditor", { image_position.x, image_position.y }, { scale, scale });

			static math::rect rect_drag_test(0, 0, 100, 100);
			static math::rect rect_drag_test2(100, 0, 100, 100);
			drag_resizable_rect("RectDrag", &rect_drag_test);
			drag_resizable_rect("RectDrag1", &rect_drag_test2);

			end_editor();
			
			if (ImGui::IsItemHovered())
			{
				// Zoom with ctrl and mousewheel
				if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0)
				{
					*zoom += ImGui::GetIO().MouseWheel;

					const float scale = std::powf(2, (*zoom)) * (ImGui::GetIO().MouseWheel > 0 ? 1 : -1);
					ImGui::SetScrollX(ImGui::GetScrollX() + (ImGui::GetScrollX() * scale) / *zoom);
					ImGui::SetScrollY(ImGui::GetScrollY() + (ImGui::GetScrollY() * scale) / *zoom);
				}
				// Hold middle mouse button to scroll
				else if (ImGui::IsMouseDown(2))
				{
					ImGui::SetScrollX(ImGui::GetScrollX() - ImGui::GetIO().MouseDelta.x);
					ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y);
				}
			}
		}
		ImGui::End();

		// Single master window test
		/*ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("Hello, world!", 0,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings
			);

		ImGui::BeginChild("somechild", ImVec2(400, 0), true);

		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("somechild2", ImVec2(0, 0), true);

		ImGui::Text("Hello.");
		ImGui::EndChild();

		ImGui::End();*/

		if (updates_enabled)
			root_node->send_down("on_postupdate", delta);

		// Send renderer events
		root_node->send_down("on_render", &myrenderer);
		myrenderer.render();

		// Render ImGui
		ImGui::Render();
		int display_w, display_h;
		glfwMakeContextCurrent(window);
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.15f, 0.15f, 0.15f, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	// Cleanup ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup GLFW
	glfwDestroyWindow(window);
	glfwTerminate();
}
