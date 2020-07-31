--------
-- Scene
--------

-- Spawn a new object into the current layer.
create_instance(layer, path)

-- Get the current delta of the frame.
delta -> float

---------
-- Object
---------

-- Remove the object from the scene.
destroy()

-- Set the position of the object.
set_position(vec2)
-- Get the vec2 position of the object.
get_position() -> vec2
-- Move the object as if calling: set_position(get_position() + offset)
move(vec2)

-- Set the scale of the sprite.
set_scale(vec2)
-- Get the scale of the sprite.
get_scale() -> vec2

-- Set the rotation of the sprite.
set_rotation(vec2)
-- Get the rotation of the sprite.
get_rotation() -> float

-- Play sprite animation.
animation_play()
-- Stop sprite animation.
animation_stop()

-- Current layer of this object.
this_layer -> layer

-----------
-- Graphics
-----------

-- Main camera for the game.
graphics.main_camera
-- (vec2) Center focus point of the camera
graphics.main_camera.focus -> vec2
-- (vec2) Max size of the view.
--   It will be adjusted automatically to fit
--   the aspect ratio of the viewport.
graphics.main_camera.size -> vec2

--------
-- Input
--------

-- Keys
key_a..z
key_left
key_right
key_up
key_down
key_space
key_tab
key_lshift
key_rshift
key_lctrl
key_rctrl
key_lalt
key_ralt

-- Mouse buttons
mouse_button_left
mouse_button_middle
mouse_button_right

-- Key states
is_key_down(key) -> boolean
is_key_pressed(key) -> boolean
is_key_released(key) -> boolean

-- Mouse button states
is_mouse_down(button) -> boolean
is_mouse_pressed(button) -> boolean
is_mouse_released(button) -> boolean

-- Mouse
-- Delta of the mouse is world coordinates.
mouse_world_delta -> vec2
-- Position of the mouse is world coordinates.
mouse_world_position -> vec2
-- Delta of the mouse is screen coordinates.
mouse_screen_delta -> vec2
-- Position of the mouse is screen coordinates.
mouse_screen_position -> vec2

-------
-- Math
-------

-- Constant 3.1415...
math.pi -> float
-- Compute sine of x in radians
math.sin(x) -> float
-- Compute cosine of x in radians
math.cos(x) -> float
-- Computes distance from 0.
-- Always returns positive x
math.abs(x) -> float
-- Rounds x down to the nearest integer
math.floor(x) -> float
-- Rounds x to the nearest integer
math.round(x) -> float
-- Returns the smaller of 2 values
math.min(x, y) -> float
-- Returns the larger of 2 values
math.max(x, y) -> float
-- Clamp x within range [min, max]
math.clamp(x, min, max) -> float
-- Computes sqaure root of x
math.sqrt(x) -> float
-- Computes the remainder of  x/y
math.mod(x, y) -> float
-- Computes a raised to the power of b
math.pow(a, b) -> float
-- e raised to the power of x
math.exp(x) -> float
-- 2 raised to the power of x
math.exp2(x) -> float
-- Computes natural logarithm (base e)
math.log(x) -> float
-- Computes base 2 logarithm
math.log2(x) -> float
-- Computes common logarithm (base 10)
math.log10(x) -> float
-- Returns true if x is not a number.
-- Dodgy divides can cause cascading NANs so
-- use this to stop them early on.
math.is_nan(x) -> boolean

-- Linearly interpolate between x and y by an amount
-- Also works with vec2
math.lerp(x, y, amount) -> float

-- Vec2
-- Construct a zero-ed vector. Same as math.vec2(0, 0)
math.vec2() -> vec2
-- Construct a copy of another vec2
math.vec2(copyvec2) -> vec2
-- Construct a vec2 with values x and y
math.vec2(x, y) -> vec2
-- Access the x field
math.vec2.x -> float
-- Access the y field
math.vec2.y -> float
-- Apply the abs function component-wise
math.vec2:abs() -> vec2
-- Get angle of of the vector in degrees
math.vec2:angle() -> float
-- Get agle from this vector to another in degrees
math.vec2:angle_to(to) -> float
-- Create a copy of this vector
math.vec2:clone() -> vec2
-- Compute the distance from this vector to another
math.vec2:distance(to) -> float
-- Compute the dot product between this vector and another
math.vec2:dot(other) -> float
-- Returns true if either component is NAN
math.vec2:is_nan() -> boolean
-- Returns true if both components are zero
math.vec2:is_zero() -> boolean
-- Returns the length of the vector or the distance from (0, 0)
math.vec2:magnitude() -> float
-- Negate the x component. (-x, y)
math.vec2:mirror_x() -> vec2
-- Negate the y component. (x, -y)
math.vec2:mirror_y() -> vec2
-- Negate both component. (*x, -y)
math.vec2:mirror_xy() -> vec2
-- Get the normal vector from this vector to another. Same as "(other - self).normalize()"
math.vec2:normal_to(vec2) -> vec2
-- Normalize the vector. Returns a vector with the same director but with a length of 1.
math.vec2:normalize() -> vec2
-- Compute vector projection
math.vec2:project(other) -> vec2
-- Compute the reflection
math.vec2:reflect(normal) -> vec2
-- Rotate around (0, 0)
math.vec2:rotate(deg) -> vec2
-- Rotate around an origin
math.vec2:rotate_around(deg, origin) -> vec2
-- Set the vector
math.vec2:set(to) -> self
-- Swap the x and y components. (y, x)
math.vec2:swap_xy() -> vec2

-- [deprecated] Compute the dot product between 2 vectors
math.dot(vec2a, vec2b) -> float
-- [deprecated] Compute the magnitude of a vector
math.magnitude(vec2) -> float
-- [deprecated] Compute the distance between 2 vectors
math.distance(vec2a, vec2b) -> float

----------
-- Physics
----------

-- Some notes:
-- Incomplete and missing things.
-- Might consider using "physics." instead of "physics_".
--   There is no difference functionally but it might be more consistant with the math API.

-- Hit info is a table that describes the hit point of a raycast.
hit_info = {
    object, -- (object handle) Handle to the object that was hit.
            --   Note: this may be nil if the object is a plain sprite asset.
    point,  -- (vec2) Hit point in world coordinates.
    normal, -- (vec2) Normal of the edge of the object that was hit.
}

-- Sends a ray from pointa to pointb. Returns true if the ray hit something, else false.
physics_raycast(hit_info, pointa, pointb) -> boolean
-- Usage:
    local hit_info = {}
    local pointa = math.vec2(0, 0)
    local pointb = math.vec2(1, 0)
    if physics_raycast(hit_info, pointa, pointb) then
        dprint("hit at " .. tostring(hit_info.point))
    end

-- Calls func with function(hit_info) for each object that was hit by the ray.
-- Note: There is no particular order to the objects.
physics_raycast_each(func, pointa, pointb) -> boolean
-- Usage:
    local pointa = math.vec2(0, 0)
    local pointb = math.vec2(1, 0)
    local callback = function(hit_info)
        dprint("hit at " .. tostring(hit_info.point))
        return true -- Returning false will cancel the raycast.
    end
    physics_raycast(callback, pointa, pointb)


-- Returns true if there is a collision in the aabb specified.
-- Min is the top-left corner of the box and max is the bottom-right corner.
physics_test_aabb(min, max) -> boolean
