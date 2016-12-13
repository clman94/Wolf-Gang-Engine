
// Everything that is implemented internally in the engine are documented here.


/// \weakgroup Entity
/// \{
///

/// Basic entity reference.
/// Nothing special about it other than
/// it keeps a reference to an entity stored in
/// the engine.
///
/// Entities are basic graphical objects in the engine. They are typically
/// very dynamic and should be used frequently as a building block. Entities come in
/// different flavours including sprite(plain and simple), character, and text.
///
/// Since this is only a reference, assigning 2 entity objects together
/// will result in 2 entity objects that point to the same entity. Which means they would
/// be modifying the same entity.
///
/// Deleting this object will not delete the entity
/// it references. You will need to use remove_entity.
///
/// \see add_entity
/// \see add_character
/// \see add_text
class entity
{
};

/// A vector type for representing coordinates.
class vec
{
	/// Default (0, 0).
	vec();
	
	/// Construct vector with values.
	vec(float pX, float pY);
	
	/// Copy another vector at construction.
	vec(const vec&in pCopy);
	
	/// Find distance from (0, 0).
	float distance() const;
	
	/// Find distance from pFrom.
	float distance(const vec&in pFrom) const;
	
	/// Find distance from (0, 0).
	/// Manhattan distance gives a kind-of diamond shape. 
	/// This should be used when distance is causing
	/// performance issues.
	float manhattan() const;
	
	/// Find distance from pFrom.
	/// Manhattan distance gives a kind-of diamond shape. 
	/// This should be used when distance is causing
	/// performance issues.
	float manhattan(const vec&in pFrom) const;
	
	/// Rotate this vector around (0, 0).
	vec& rotate(float pDegrees);
	
	/// Rotate this vector around pOrigin.
	vec& rotate(const vec&in pOrigin, float pDegrees);
	
	/// Normalize this vector.
	/// In simpler terms, this "cuts" this vector
	/// so its distance is exactly 1.
	///
	/// `vec(2, 0).normalize() == vec(1, 0)`
	vec& normalize();
	
	float x; ///< X coordinate
	float y; ///< Y coordinate
};

/// Create a new sprite-based entity.
/// Default atlas is "default:default"
entity add_entity(const string&in pTexture);

/// Create a new sprite-based entity with a specific atlas.
entity add_entity(const string&in pTexture, const string&in pAtlas);

/// Create a text-based entity.
/// This will use the main font set in game.xml.
entity add_text();

/// Set text of a text-based entity.
void set_text(entity&in pEntity, const string &in pText);

/// Add a sprite-based entity specialized as a character.
/// Character entities include walk cycles and direction.
entity add_character(const string&in pTexture);

/// Set position of entity.
void set_position(entity&in pEntity, const vec&in pVec);

/// Get position of entity.
vec get_position(entity&in pEntity);

/// Set the direction of a character entity.
void set_direction(entity&in pEntity, direction pDirection);

/// Set walk cycle of character entity.
/// Walk cycles are defined in atlases with `name:`.
void set_cycle(entity&in pEntity, const string &in);

/// Start animation of entity (if there is one)
void start_animation(entity&in pEntity);

/// Stop animation of entity if it is playing
void stop_animation(entity&in pEntity);

/// Set animation/atlas of entity.
void set_animation(entity&in pEntity, const string &in pAtlas);

/// Find an entity by its name.
/// \see set_name
entity find_entity(const string &in pName);

/// Check if entity is a character type.
bool is_character(entity&in pEntity);

/// Delete an entity.
/// Only effective if the entity was spawned with `add_entity`,
/// `add_character`, `add_text`, etc...
void remove_entity(entity&in pEntity);

/// Set specific depth of entity. Will automatically disable dynamic depth.
/// A larger value results in the entity being farther behind.
// All values are 0-255.
void set_depth(entity&in pEntity, float pDepth);

/// Set dynamic depth in entity.
/// Dynamic depth cause an entity to automatically adjust its depth when its
/// Y coordinate changes. This is useful when there are characters walking and they
/// can "walk though" other character/entities.
void set_depth_fixed(entity&in pEntity, bool pIs_fixed);

/// Set name of entity.
/// All entities default at a blank name.
/// If you need to specify an entity and reference it later,
/// this might be useful.
void set_name(entity&in pEntity, const string &in pName);

/// Set rotation of entity.
void set_rotation(entity&in pEntity, float pDegrees);

/// Set color of entity.
void set_color(entity&in pEntity, int R, int G, int B, int A);

/// Set the visibility of entity.
void set_visible(entity&in pEntity, bool pIs_visible);

/// Set texture of entity.
void set_texture(entity&in pEntity, const string&in pTexture);

/// Add child of entity.
/// The child entity will "follow" the parent.
void add_child(entity&in pEntity, entity&in pChild);

/// Set the parent of entity.
/// The child entity will "follow" the parent.
void set_parent(entity&in pEntity, entity&in pParent);

/// Detach all children (if are any).
void detach_children(entity&in pEntity);

/// Detach parent (if there is one).
void detach_parent(entity&in pEntity);

/// Prepare an entity to be used for GUI.
/// The entity will overlay everything else and
/// be disconnected from the world node.
/// \param pOrder Higher value will result in the entity being above and lower value below.
void make_gui(entity&in pEntity, float pOrder);

/// Get the entity of the player character.
///
entity get_player();
/// \}


/// \weakgroup Scene
/// \{

/// Set location for camera to focus on.
/// The camera will automatically stay within the boundaries of the scene.
/// Automatically sets player::focus to false.
void set_focus(vec pPosition);

/// Get focus of camera.
vec get_focus();

/// Set a tile in the tilemap.
/// Any overlapping tiles will be replaced.
/// The tilemap is optimized for speed and not flexibility so it is recommended
/// use entities over the tilemap any time you can.
/// \param pAtlas The atlas entry in the selected tilemap texture.
/// \param pPosition Position of tile. Offsets(decimal) is allowed but it might be awkward to work with.
/// \param pLayer Higher value will result in the tile closer to the camera and farther if lower.
/// \param pRotation Simple value from 0-3?: 0 = 0 degrees; 1 = 90 degrees; 2 = 180 degrees; 3 = 270 degrees.
void set_tile(const string&in pAtlas, vec pPosition, int pLayer, int pRotation);

/// Remove tile at position.
void remove_tile(vec pPosition, int pLayer);

/// Get position of boundary region.
vec get_boundary_position();

/// Get size of boundary region.
vec get_boundary_size();

/// Set position of boundary region.
void get_boundary_position(vec pPosition);

/// set size of boundary region.
void get_boundary_size(vec pSize);

/// Set whether or not the boundary is used.
void set_boundary_enable(bool pIs_enabled);

/// Load scene from file
/// \param pPath Path to XML file
void load_scene(const string &in pPath);

/// \}

/// \weakgroup Game
/// \{

/// Print message for debugging.
void dprint(const string &in pMessage);

/// Print error message.
void eprint(const string &in pMessage);

/// Get delta (time between each frame).
/// This can be used as a scale for movement or changes over time so then each
/// frame is consistent.
///
///  Ex. `set_position(my_entity, get_position(my_entity) + (move*speed*get_delta()));``
float get_delta();

/// Set current save slot.
/// This can be set once and it will stay the same for
/// the rest of the game. This allows you to call save_game/open_game 
/// during the game without worrying about which slot to use (like checkpoint and such).
void set_slot(uint pSlot);

/// Get currently used slot.
uint get_slot();

/// Save game to the current slot.
/// Flags, current scene, and player position
/// are currently the only values saved.
void save_game();

/// Open save from current save slot.
void open_game();

/// Check if a save slot is being used.
bool is_slot_used(uint);

/// Store a handle globally so other scenes can
/// receive it.
/// The data will have be a [shared entity](http://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_shared.html).
///
/// File 1:
/// ```
/// shared class my_type
/// {
///    int value;
/// };
/// 
/// [start]
/// void share_the_data()
/// {
///    my_type data;
///    data.value = 0;
///    make_shared(@data, "mydata"); // Share a handle to all scenes
/// }
/// ```
///
/// File 2:
/// ```
/// shared class my_type
/// {
///    int value;
/// };
/// 
/// [start]
/// void print_value()
/// {
///    // Retrieve the handle and cast it to the desired type.
///    my_type@ data = cast<my_type>(get_shared("mydata"));
///    if (data is null) // Failed to retrieve data
///    {
///       eprint("Wrong value!");
///       return;
///    }
///    dprint(formatInt(data.value));
/// }
/// ```
/// \param pData Handle of the object.
/// \param pName Name to refer to data with.
/// \see get_shared
void make_shared(ref@ pData, const string&in pName);

/// Retrieve handle of data
/// \param pName Name to refer to data with
/// \see make_shared
ref@ get_shared(const string&in pName);

/// \}
