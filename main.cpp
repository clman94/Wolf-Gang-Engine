














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

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Angelscript
#include <angelscript.h>

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
using namespace wge;

#include <Box2D/Box2D.h>


std::string load_file_as_string(const std::string& pPath);

// Loadable assets are extra files that
// need to be loaded before they can be
// used. E.g. images, sounds, etc...
//
// These assets need to be referenced by an asset
// configuration file before they are useful.
class asset_loadable
{
public:
	virtual ~asset_loadable(){}

	// Load the resource to memory
	virtual void load() = 0;
	// Unload this resource and free memory
	virtual void unload() = 0;
	// Returns true if the resource is loaded
	virtual bool is_ready() const = 0;

	virtual const std::string& get_name() const = 0;
};

typedef uint64_t asset_id_t;

// Asset files contain configuration.
//
// All asset files are parsed and cached
// for quick access during the engines runtime.
// If you modify an asset file outside the editor
// you may need to refresh the cache.
class asset
{
public:
	void set_name(const std::string& pName)
	{
		mName = pName;
	}

	const std::string& get_name() const
	{
		return mName;
	}

	void load(const std::string& pName)
	{
		json j(load_file_as_string(pName));

		mName = j["name"].get<std::string>();
		mID = j["id"];
		mDescription = j["description"].get<std::string>();

		for (const json& i : j["resources"])
			mLinked_resources.push_back(i);

		on_load(j["data"]);
	}

	void save(const std::string& pPath)
	{

	}

protected:
	virtual void on_load(const json& pData) {}
	virtual json on_save() { return {}; }

private:
	std::string mName;
	std::string mDescription;
	std::vector<std::string> mLinked_resources;
	asset_id_t mID;
};

// Loads non-config file as an asset_loadable
class asset_loader
{
public:

};

class asset_manager
{
public:
	// TODO: Load and cache all configuration files.
};


/*

Events sent:
	void on_physics_update_bodies(physics_world_component*);
	  - Used by physics_component's to update their bodies to
	    the current world.

Events recieved:
	void on_preupdate(float);
*/
class physics_world_component :
	public core::component
{
	WGE_COMPONENT("Physics World", 3)
public:

	physics_world_component(core::object_node* pObj) :
		component(pObj)
	{
		mWorld = new b2World(b2Vec2(0, 1));
		mWorld->SetContactListener(&mContact_listener);
		subscribe_to(pObj, "on_preupdate", &physics_world_component::on_preupdate);
	}

	virtual ~physics_world_component()
	{
		delete mWorld;
	}


	void on_add_node(core::object_node* pObj)
	{
		if (pObj->has_component<physics_component>())
		{
			physics_component * phys = pObj->get_component<physics_component>();
			// Take ownership of the body if it hasn't been claimed already
			if (!phys->mBody)
				phys->mBody = mWorld->CreateBody(&phys->mBody_def);
		}
	}

	void on_remove_node(core::object_node* pObj)
	{
		if (pObj->has_component<physics_component>())
		{
			physics_component * phys = pObj->get_component<physics_component>();
			//assert(phys && phys->mBody);
			mWorld->DestroyBody(phys->mBody);
			phys->mBody = nullptr;
		}
	};

	void set_gravity(math::vec2 pVec)
	{
		mWorld->SetGravity({ pVec.x, pVec.y });
	}

	math::vec2 get_gravity() const
	{
		b2Vec2 gravity = mWorld->GetGravity();
		return{ gravity.x, gravity.y };
	}

	b2Body* create_body(const b2BodyDef& pDef)
	{
		return mWorld->CreateBody(&pDef);
	}

private:
	struct contact
	{
		enum {
			state_begin,
			state_continued,
			state_end
		};
		int state;
	};

	struct contact_listener :
		b2ContactListener
	{
		virtual void BeginContact(b2Contact* pContact)
		{
			b2Fixture* f1 = pContact->GetFixtureA();
			b2Fixture* f2 = pContact->GetFixtureB();
			b2Body* b1 = f1->GetBody();
			b2Body* b2 = f2->GetBody();
			if (physics_component* component =
				static_cast<physics_component*>(b1->GetUserData()))
				component->get_object()->send("on_collision_begin");
			if (physics_component* component =
				static_cast<physics_component*>(b2->GetUserData()))
				component->get_object()->send("on_collision_begin");
		}
	};

	void on_preupdate(float pDelta)
	{
		// Update the bodies
		get_object()->send_down("on_physics_update_bodies", this);

		// Calculate physics
		mWorld->Step(pDelta, 1, 1);
	}

private:
	b2World* mWorld;
	contact_listener mContact_listener;

	friend class physics_component;
};

class physics_component :
	public core::component
{
	WGE_COMPONENT("Physics", 5431);
public:
	physics_component(core::object_node* pObj) :
		component(pObj)
	{
		mBody = nullptr;
		mBody_def.userData = get_object();
		mBody_def.type = b2BodyType::b2_dynamicBody;
		subscribe_to(pObj, "on_physics_update_bodies", &physics_component::on_physics_update_bodies);
	}


private:
	void on_physics_update_bodies(physics_world_component* pComponent)
	{
		if (!mBody)
		{
			auto transform = get_object()->get_component<core::transform_component>();
			assert(transform);
			mBody_def.position.x = transform->get_absolute_position().x;
			mBody_def.position.y = transform->get_absolute_position().y;
			mBody_def.angle = transform->get_absolute_rotation();
			mBody = pComponent->create_body(mBody_def);
		}
	}

private:
	std::vector<std::weak_ptr<core::object_node>> mObjects_with_collision;
	b2Body* mBody;
	b2BodyDef mBody_def;
	friend class physics_world_component;
};

class physics_box_collider :
	public core::component
{
	WGE_COMPONENT("Box Collider", 268);
public:
	physics_box_collider(core::object_node* pObj) :
		component(pObj)
	{
		mShape.SetAsBox(50, 50);
		mFixture_def.shape = &mShape;
		subscribe_to(pObj, "on_physics_update_colliders", &physics_box_collider::on_physics_update_colliders);
	}

private:

	void on_physics_update_colliders()
	{

	}

private:
	b2Fixture* mFixture;
	b2FixtureDef mFixture_def;
	b2PolygonShape mShape;
};

std::string load_file_as_string(const std::string& pPath)
{
	std::ifstream stream(pPath.c_str());
	if (!stream.good())
	{
		std::cout << "Could not load file \"" << pPath << "\"\n";
		std::getchar();
		return{};
	}
	std::stringstream sstr;
	sstr << stream.rdbuf();
	return sstr.str();
}

bool compile_shader(GLuint pGL_shader, const std::string& pSource)
{
	// Compile Shader
	const char * source_ptr = pSource.c_str();
	glShaderSource(pGL_shader, 1, &source_ptr, NULL);
	glCompileShader(pGL_shader);

	// Get log info
	GLint result =  GL_FALSE;
	int log_length = 0;
	glGetShaderiv(pGL_shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(pGL_shader, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0)
	{
		// A compilation error occured. Print the message.
		std::vector<char> message(log_length + 1);
		glGetShaderInfoLog(pGL_shader, log_length, NULL, &message[0]);
		printf("%s\n", &message[0]);
		return false;
	}

	return true;
}

GLuint load_shaders(const std::string& pVertex_path, const std::string& pFragment_path)
{
	// Load sources
	std::string vertex_str = load_file_as_string(pVertex_path);
	std::string fragment_str = load_file_as_string(pFragment_path);

	if (vertex_str.empty() || fragment_str.empty())
		return 0;

	// Create the shader objects
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Compile the shaders
	compile_shader(vertex_shader, vertex_str);
	compile_shader(fragment_shader, fragment_str);

	// Link the shaders to a program
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);
	glLinkProgram(program_id);

	// Check for errors
	GLint result = GL_FALSE;
	int log_length = 0;
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0)
	{
		// An error occured
		std::vector<char> message(log_length + 1);
		glGetProgramInfoLog(program_id, log_length, NULL, &message[0]);
		printf("%s\n", &message[0]);
		return 0;
	}

	// Cleanup

	glDetachShader(program_id, vertex_shader);
	glDetachShader(program_id, fragment_shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program_id;
}

void show_info(stb_vorbis *v)
{
	if (v) {
		stb_vorbis_info info = stb_vorbis_get_info(v);
		printf("%d channels, %d samples/sec\n", info.channels, info.sample_rate);
		printf("Predicted memory needed: %d (%d + %d)\n", info.setup_memory_required + info.temp_memory_required,
			info.setup_memory_required, info.temp_memory_required);
	}
}

class framebuffer
{
public:

	framebuffer()
	{
		mWidth = 0;
		mHeight = 0;
	}

	~framebuffer()
	{
		glDeleteFramebuffers(1, &mFramebuffer);
	}

	void create(int pWidth, int pHeight)
	{
		glGenFramebuffers(1, &mFramebuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

		// Create the texture
		glGenTextures(1, &mTexture);
		glBindTexture(GL_TEXTURE_2D, mTexture);

		// Create the texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pWidth, pHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		// GL_NEAREST will tell the sampler to get the nearest pixel when
		// rendering rather than lerping the pixels together.
		// It may be better to use a GL_LINEAR filter so then upscaling/downscaling
		// isn't so jaring.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// Attach the texture to #0
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 0);

		// Set the draw buffer
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "Incomplete framebuffer\n";
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		mWidth = pWidth;
		mHeight = pHeight;
	}

	void resize(int pWidth, int pHeight)
	{
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pWidth, pHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		mWidth = pWidth;
		mHeight = pHeight;
	}

	// This sets the frame buffer for opengl.
	// Call first if you want to draw to this framebuffer.
	// Call end_framebuffer() when you are done.
	void begin_framebuffer() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
	}

	// Resets back to the default framebuffer
	void end_framebuffer() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// Get the raw gl buffer for rendering
	GLuint get_gl_texture() const
	{
		return mTexture;
	}

	int get_width() const
	{
		return mWidth;
	}

	int get_height() const
	{
		return mHeight;
	}

private:
	GLuint mTexture, mFramebuffer;
	int mWidth, mHeight;
};

class texture
{
public:
	void open(const std::string& pFilepath)
	{
		stbi_uc* pixels = stbi_load(pFilepath.c_str(), &mWidth, &mHeight, &mChannels, 4);
		if (!pixels)
		{
			std::cout << "Failed to open image\n";
			return;
		}

		// Create the texture object
		glGenTextures(1, &mGL_texture);
		glBindTexture(GL_TEXTURE_2D, mGL_texture);

		// Give the image to OpenGL
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	GLuint get_gl_texture()
	{
		return mGL_texture;
	}


private:
	int mChannels;
	int mWidth, mHeight;
	GLuint mGL_texture;
};

struct vertex_2d
{
	math::vec2 position;
	math::vec2 uv;
};

struct render_batch_2d
{
	texture* rendertexture;

	std::vector<unsigned int> indexes;
	std::vector<vertex_2d> vertices;

	render_batch_2d() :
		rendertexture(nullptr)
	{}
};

class batch_builder
{
public:

	// TODO: Add ability to draw to several framebuffers for post-processing
	//void set_framebuffer(const std::string& pName);

	void set_texture(texture* pTexture)
	{
		mBatch.rendertexture = pTexture;
	}

	// Returns a pointer to 4 vertices
	vertex_2d* push_quad()
	{
		std::size_t start_index = mBatch.vertices.size();

		// Make sure there is enough space
		mBatch.indexes.reserve(mBatch.indexes.size() + 6);

		// Triangle 1
		mBatch.indexes.push_back(start_index);
		mBatch.indexes.push_back(start_index + 1);
		mBatch.indexes.push_back(start_index + 2);

		// Triangle 2
		mBatch.indexes.push_back(start_index + 2);
		mBatch.indexes.push_back(start_index + 3);
		mBatch.indexes.push_back(start_index);

		// Create the new vertices and return the pointer to the first one
		mBatch.vertices.resize(mBatch.vertices.size() + 4);
		return &mBatch.vertices[start_index];
	}

	render_batch_2d* get_batch()
	{
		return &mBatch;
	}

private:
	render_batch_2d mBatch;
};

class renderer
{
public:
	void initialize(int pFramebuffer_width, int pFramebuffer_height)
	{
		mFramebuffer.create(pFramebuffer_width, pFramebuffer_height);
		mShader = load_shaders("./vert.glsl", "./frag.glsl");

		// Create the needed vertex buffer
		glGenBuffers(1, &mVertex_buffer);

		// Allocate ahead of time
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_2d) * 4, NULL, GL_STATIC_DRAW);

		// Create the element buffer to hold our indexes
		glGenBuffers(1, &mElement_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6, NULL, GL_STATIC_DRAW);
	}

	void set_framebuffer_size(int pWidth, int pHeight)
	{
		mFramebuffer.resize(pWidth, pHeight);
	}

	void push_batch(const render_batch_2d& pBatch)
	{
		mBatches.push_back(pBatch);
	}

	void render_batch(const render_batch_2d& pBatch)
	{
		// Populate the vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, pBatch.vertices.size()*sizeof(vertex_2d), &pBatch.vertices[0], GL_STATIC_DRAW);

		// Populate the element buffer with index data
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pBatch.indexes.size() * sizeof(unsigned int), &pBatch.indexes[0], GL_STATIC_DRAW);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pBatch.rendertexture->get_gl_texture());

		GLuint tex_id = glGetUniformLocation(mShader, "tex");
		glUniform1i(tex_id, 0);

		// Setup the 2d position attribute
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glVertexAttribPointer(
			0,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(vertex_2d),
			(void*)0
		);

		// Setup the UV attribute
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glVertexAttribPointer(
			1,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(vertex_2d),
			(void*)sizeof(math::vec2) // Offset to the uv components in vertex_2d
		);

		// Bind the element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);

		glDrawElements(
			GL_TRIANGLES,
			pBatch.indexes.size(),
			GL_UNSIGNED_INT,
			(void*)0
		);

		// Disable all the attributes
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);
	}

	void render()
	{
		mFramebuffer.begin_framebuffer();

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, mFramebuffer.get_width(), mFramebuffer.get_height());

		glUseProgram(mShader);

		// Create the projection matrix
		math::mat44 proj_mat = math::ortho(0, mFramebuffer.get_width(), 0, mFramebuffer.get_height());

		// This matrix transforms the object itself
		math::mat44 model_mat{ 1 };

		// Set the matrix uniforms
		GLuint proj_id = glGetUniformLocation(mShader, "projection");
		glUniformMatrix4fv(proj_id, 1, GL_FALSE, &proj_mat.m[0][0]);
		GLuint model_id = glGetUniformLocation(mShader, "model");
		glUniformMatrix4fv(model_id, 1, GL_FALSE, &model_mat.m[0][0]);

		// Render the batches
		for (const auto& i : mBatches)
			render_batch(i);

		// cleanup
		glUseProgram(0);
		mFramebuffer.end_framebuffer();
		mBatches.clear();
	}

	const framebuffer& get_framebuffer() const
	{
		return mFramebuffer;
	}

private:
	framebuffer mFramebuffer;
	GLuint mShader;
	GLuint mVertex_buffer, mElement_buffer;

	std::vector<render_batch_2d> mBatches;
};

class sprite_component :
	public core::component
{
	WGE_COMPONENT("Sprite", 12409);
public:
	sprite_component(core::object_node* pNode) :
		component(pNode)
	{
		mTexture.open("./mytex.png");
		subscribe_to(pNode, "on_render", &sprite_component::on_render, this);
	}

	void on_render(renderer& pRenderer)
	{
		core::transform_component* transform = get_object()->get_component<core::transform_component>();

		batch_builder batch;

		batch.set_texture(&mTexture);

		vertex_2d* verts = batch.push_quad();

		verts[0].position = math::vec2(0, 0);
		verts[0].uv = math::vec2(0, 0);
		verts[1].position = math::vec2(100, 0);
		verts[1].uv = math::vec2(1, 0);
		verts[2].position = math::vec2(100, 100);
		verts[2].uv = math::vec2(1, 1);
		verts[3].position = math::vec2(0, 100);
		verts[3].uv = math::vec2(0, 1);

		for (int i = 0; i < 4; i++)
			verts[i].position = transform->get_absolute_transform() * (verts[i].position + mOffset);

		pRenderer.push_batch(*batch.get_batch());
	}

	math::vec2 get_offset() const
	{
		return mOffset;
	}

	void set_offset(const math::vec2& pOffset)
	{
		mOffset = pOffset;
	}

private:
	texture mTexture;
	math::vec2 mOffset;
};

// Failed attempt at using the stb_vorbis pushdata api.
// The pushdata api allows the user to give the stream a chunk of data and
// it spits out samples.
// While I do understand the advantage of this api, it is rather unstable
// when resyncing and requires the user to implement what is already implemented
// in the other api.
//
// This library should opt for a callback system like what was used in stb_image
// instead of hard coding in stdio.
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
					// Seek the stream to the beginning for looping
					// Possible issue: There could be chance there will only
					// be a few samples left and when the source gets to processing
					// that buffer, we wont have enough time to actually process the 
					// next one.
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
void Image(const framebuffer& mFramebuffer, const ImVec2& pSize = ImVec2(0, 0))
{
	ImGui::Image((void*)mFramebuffer.get_gl_texture(), pSize,
		ImVec2(0, 1), ImVec2(1, 0)); // Y-axis needs to be flipped
}

}

#include <scriptbuilder/scriptbuilder.h>
#include <scripthandle/scripthandle.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>
#include <contextmgr/contextmgr.h>

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
// Note: When you want to pass a argument as a handle, remember to put a @ in front it. e.g. "coroutine::start("void func(clazz@)", @myclazz)"
//   If you forget to, the object will be copied.
//

// This copies an argument to another parameter in angelscript. May still need some work as it hasn't been fully tested in all situations.
void copy_argument(asIScriptEngine* engine, void* dest, int dest_typeid, void* src, int src_typeid)
{
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
	CContextMgr manager;

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
		asIScriptEngine* engine = reinterpret_cast<asIScriptEngine*>(pGen->GetAuxiliary());
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
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in)", asFUNCTION(varfunc), asCALL_GENERIC, engine);
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in, ?&in)", asFUNCTION(varfunc), asCALL_GENERIC, engine);
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in, ?&in, ?&in)", asFUNCTION(varfunc), asCALL_GENERIC, engine);
	engine->RegisterGlobalFunction("void varfunc(const ref&in, const string&in, ?&in, ?&in, ?&in)", asFUNCTION(varfunc), asCALL_GENERIC, engine);

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
		if (auto func = mInspector_guis[pComponent->get_id()])
			func(pComponent);
	}

private:
	std::map<int, std::function<void(core::component*)>> mInspector_guis;
};

void show_node_tree(util::ref<core::object_node> pNode, editor_component_inspector& pInspector)
{
	if (ImGui::TreeNode(pNode->get_name().c_str()))
	{
		// List out the components
		for (std::size_t i = 0; i < pNode->get_component_count(); i++)
		{
			core::component* comp = pNode->get_component_index(i);
			if (ImGui::CollapsingHeader(comp->get_name().c_str()))
				pInspector.on_gui(comp);
		}

		// Show the children nodes and their components
		for (std::size_t i = 0; i < pNode->get_child_count(); i++)
			show_node_tree(pNode->get_child(i), pInspector);

		ImGui::TreePop();
	}
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
	ImGui::BeginChild(pTitle, pSize, true);
	ImDrawList* dl = ImGui::GetOverlayDrawList();

	// Draw the background
	dl->AddRectFilled(ImGui::GetWindowPos(),
		ImVec2(ImGui::GetWindowWidth() + ImGui::GetWindowPos().x, 20.f + ImGui::GetWindowPos().y),
		ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]),
		ImGui::GetStyle().ChildRounding, ImDrawCornerFlags_Top);

	// Draw the title
	dl->AddText(ImVec2(ImGui::GetWindowPos().x + 10, ImGui::GetWindowPos().y + 4),
		ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]),
		pTitle);
}

}

int main()
{
	std::cout << filesystem::path("pie/int.exe").string();

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


	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	use_default_style();

	// A random cursor test
	unsigned char pixels[16 * 16 * 4];
	memset(pixels, 0xff, sizeof(pixels));
	GLFWimage image;
	image.width = 16;
	image.height = 16;
	image.pixels = pixels;
	GLFWcursor* cursor = glfwCreateCursor(&image, 0, 0);
	glfwSetCursor(window, cursor);

	math::mat44 proj_mat = math::ortho(0, 200, 0, 200);
	//proj_mat.rotate(math::pi*0.4);
	std::cout << proj_mat.to_string();

	math::mat44 model_mat{ 1 };
	model_mat.translate(3, 3);
	model_mat.rotate(math::degrees(3));

	GLuint shader_program = load_shaders("./vert.glsl", "./frag.glsl");

	texture testtexture;
	testtexture.open("./mytex.png");

	glUseProgram(shader_program);
	GLuint proj_id = glGetUniformLocation(shader_program, "projection");
	glUniformMatrix4fv(proj_id, 1, GL_FALSE, &proj_mat.m[0][0]);
	GLuint model_id = glGetUniformLocation(shader_program, "model");
	glUniformMatrix4fv(model_id, 1, GL_FALSE, &model_mat.m[0][0]);

	GLuint vertex_array_id;
	glGenVertexArrays(1, &vertex_array_id);
	glBindVertexArray(vertex_array_id);

	const GLfloat g_vertex_buffer_data[] = {
		0, 0, 0.0f,
		100, 105, 0.0f,
		100,  0, 0.0f,
	};

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	framebuffer myframebuffer;
	myframebuffer.create(200, 200);

	myframebuffer.begin_framebuffer();

	glClearColor(0, 0, 0.4f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, 200, 200);

	glUseProgram(shader_program);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisableVertexAttribArray(0);

	myframebuffer.end_framebuffer();

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
	inspector_guis.add_inspector(sprite_component::COMPONENT_ID,
		[](core::component* pComponent)
	{
		auto sprite = dynamic_cast<sprite_component*>(pComponent);
		math::vec2 offset = sprite->get_offset();
		if (ImGui::DragFloat2("Offset", offset.components))
			sprite->set_offset(offset);
	});

	// Inspector for physics_world_component
	inspector_guis.add_inspector(physics_world_component::COMPONENT_ID,
		[](core::component* pComponent)
	{
		auto physicsworld = dynamic_cast<physics_world_component*>(pComponent);
		math::vec2 gravity = physicsworld->get_gravity();
		if (ImGui::DragFloat2("Gravity", gravity.components))
			physicsworld->set_gravity(gravity);
	});

	auto root_node = core::object_node::create();
	root_node->set_name("Game");
	{
		auto node1 = core::object_node::create();
		node1->set_name("Scene node 1");
		node1->add_component<core::transform_component>();
		node1->add_component<sprite_component>();
		root_node->add_child(node1);

		{
			auto node2 = core::object_node::create();
			node2->set_name("Scene node 2");
			node2->add_component<core::transform_component>();
			node2->add_component<sprite_component>();
			node1->add_child(node2);
		}
	}

	renderer myrenderer;
	myrenderer.initialize(200, 200);

	util::clock clock;
	while (!glfwWindowShouldClose(window))
	{
		// Calculate delta
		float delta = clock.restart();

		// Get window events
		glfwPollEvents();

		// Send update events
		root_node->send_down("on_preupdate", delta);
		root_node->send_down("on_update", delta);
		root_node->send_down("on_postupdate", delta);

		// Send renderer events
		root_node->send_down("on_render", myrenderer);
		myrenderer.render();

		// Start a new frame for ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Object Inspector");

		ImGui::BeginChild("eeeyy", ImVec2(0, 0), true);

		show_node_tree(root_node, inspector_guis);

		ImGui::EndChild();

		ImGui::End();

		ImGui::Begin("Viewport");
		if (myrenderer.get_framebuffer().get_width() != ImGui::GetWindowWidth()
			|| myrenderer.get_framebuffer().get_height() != ImGui::GetWindowHeight())
			myrenderer.set_framebuffer_size(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
		ImGui::Image(myrenderer.get_framebuffer(), ImVec2(
			ImGui::GetWindowWidth(), ImGui::GetWindowHeight()));
		ImGui::End();

		ImGui::Begin("Hello, world!");

		if (ImGui::CollapsingHeader("Style Editor"))
			ImGui::ShowStyleEditor();


		if (ImGui::CollapsingHeader("AngelScript Test"))
		{
			if (ImGui::Button("Test"))
				test_as_varfunc();
		}

		if (ImGui::CollapsingHeader("GL Test"))
		{
		}

		if (ImGui::CollapsingHeader("AL Test"))
		{
			if (ImGui::Button("Test"))
				al_test_thread = std::thread(&test_stream_al2);
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