namespace gui
{
/// \weakgroup GUI
/// Menu system.
/// Because these gui classes are rather general use,
/// here are just a few uses:
/// ```
/// gui::selection_menu sel_options;
/// sel_action.add(gui::menu_function("Option1"
///     , function(pArgs) {
///         dprint("This is Option1");
///     }));
/// 
/// gui::selection_menu sel_submenu("Sub Menu");
/// sel_action.add(gui::menu_function("Sub Option1"
///     , function(pArgs) {
///         dprint("This is Sub Option1");
///     }));
/// sel_options.add(sel_submenu); // Add the sub-menu
/// 
/// sel_options.open(); // Start the selection
/// ```
/// 
/// \{

/// Simple controller for list-based selection
class selector_control
{
	selector_control()
	{
		mCurrent_selection = 0;
		mItem_count = 0;
		mHeight = 3;
	}
	
	/// Get index of selected item
	uint get_selection()
	{
		return mCurrent_selection;
	}
	
	/// Set count of items
	void set_item_count(uint pCount)
	{
		mItem_count = pCount;
	}
	
	/// Get amount of items
	uint get_item_count()
	{
		return mItem_count;
	}
	
	/// Set height of columns. When this
	/// height is reached, the selector will
	/// create a new column.
	void set_column_height(uint pHeight)
	{
		mHeight = pHeight;
	}
	
	/// Get column height
	uint get_column_height()
	{
		return mHeight;
	}
	
	/// Reset the current selection
	void reset()
	{
		mCurrent_selection = 0;
	}
	
	/// Check controls and update selection
	uint tick()
	{
		uint offset = 0;
		
		// Check controls and calculate the offset for the new selection
		offset -= is_triggered(control::select_previous) ? mHeight : 0;
		offset += is_triggered(control::select_next)     ? mHeight : 0;
		offset -= is_triggered(control::select_up)       ? 1 : 0;
		offset += is_triggered(control::select_down)     ? 1 : 0;
		
		if (offset != 0)
		{
			uint new_selection = mCurrent_selection + offset;
			
			// Prevent the selection from going outside the array.
			// Also gives a nice effect.
			if (new_selection >= 0 &&
				new_selection < mItem_count)
				mCurrent_selection = new_selection;
		}
		return mCurrent_selection;
	}
	
	private uint mCurrent_selection;
	private uint mItem_count;
	private uint mHeight;
};

/// Simple selector for text-based lists
class basic_text_selection
{
	basic_text_selection()
	{
		mSeperation = pixel(96, 20);
		mAnchor = anchor::topleft;
	}
	
	/// Set the seperation between each item.
	/// X = Offset for columns
	/// Y = Offset for rows
	void set_seperation(vec pSeperation)
	{
		mSeperation = pSeperation;
	}
	
	void set_column_height(uint pHeight)
	{
		mSelector.set_column_height(pHeight);
	}
	
	void set_anchor(anchor pAnchor)
	{
		for (uint i = 0; i < mEntities.length(); i++)
			::set_anchor(mEntities[i], pAnchor);
		mAnchor = pAnchor;
	}
	
	uint add(const string&in pText) final
	{
		entity new_text = add_text(); // Setup the text entity
		set_text(new_text, pText);
		make_gui(new_text, 1);
		::set_visible(new_text, false);
		::set_anchor(new_text, mAnchor);
		
		// Keep track of all the entities so they can
		// be deleted when this object is deleted.
		mEntities.insertLast(scoped_entity(new_text));
		
		update();
		
		return mEntities.length() - 1; // Return index of new item
	}
	
	void remove(uint pIndex) final
	{
		if (pIndex >= mEntities.length())
		{
			eprint("Invalid index");
			return;
		}
		mEntities.removeAt(pIndex);
		
		update();
	}
	
	void remove_all()
	{
		mEntities.resize(0);
		update();
	}
	
	int start() final
	{
		set_visible(true);
		
		scoped_entity cursor = create_cursor();
		scoped_entity@ current_entity;
		
		do
		{
			mSelector.tick();
			
			@current_entity = mEntities[mSelector.get_selection()];
			
			::set_position(cursor, get_position(current_entity));
			
			yield();
			
			if (is_triggered(control::back))
			{
				set_visible(false);
				return -1;
			}
		}while(!is_triggered(control::activate));
		set_visible(false);
		
		return int(mSelector.get_selection());
	}
	
	void set_position(vec pPosition) final
	{
		mBase_position = pPosition;
		position_entities();
	}
	
	void set_visible(bool pIs_visible)
	{
		for (uint i = 0; i < mEntities.length(); i++)
			::set_visible(mEntities[i], pIs_visible);
	}
	
	protected entity create_cursor()
	{
		entity cursor = add_entity("NarrativeBox", "SelectCursor");
		::set_anchor(cursor, anchor::topright);
		make_gui(cursor, 1);
		return cursor;
	}
	
	private void position_entities()
	{
		if (mSelector.get_column_height() <= 0) // Avoid divide-by-zero
			return;
		for (uint i = 0; i < mEntities.length(); i++)
		{
			const vec position = mBase_position 
			+ vec(floor(i/mSelector.get_column_height())*mSeperation.x // Columns
			, mSeperation.y * (i % mSelector.get_column_height())); // Rows
			::set_position(mEntities[i], position);
		}
	}
	
	private void update()
	{
		position_entities();
		mSelector.set_item_count(mEntities.length());
	}
	
	private vec mBase_position;
	private vec mSeperation;
	private selector_control mSelector;
	private anchor mAnchor;
	private array<scoped_entity@> mEntities;
};



/// Base for a menu.
class menu
{
	menu(){}
	menu(const string&in pId)
	{
		mMenu_id = pId;
	}
	
	/// Open the menu
	/// Return null if there has been no selection (like going back).
	/// The menu before it will have to handle it correctly:
	/// ```
	/// menu@ ret;
	/// do{
	///   // Do its thing
	///   ret = sub_menu.open();
	/// } while(ret is null) // Keep looping until an item is selected
	///                      // or this function returns null and then
	///                      // the caller before it will have to handle this as well.
	/// return ret;
	/// ```
	menu@ open(){ return @this; }
	
	/// Get identification of menu
	string get_id()
	{
		return mMenu_id;
	}
	
	/// Set identification
	/// This string is usually used in the selection_menu
	/// for the text.
	void set_id(const string&in pId)
	{
		mMenu_id = pId;
	}
	
	private string mMenu_id;
};

/// A menu item that is selected.
class menu_item : menu
{
	menu_item()
	{
		mIs_selected = false;
	}
	
	menu_item(const string&in pId)
	{
		set_id(pId);
		mIs_selected = false;
	}
	
	menu@ open()
	{
		mIs_selected = true;
		return @this;
	}
	
	/// Check if this menu has been selected
	bool is_selected() const
	{
		return mIs_selected;
	}
	
	bool opImplConv() const
	{
		return mIs_selected;
	}
	
	protected bool mIs_selected;
};

/// Function definition of callback
funcdef void menu_function_t(dictionary@);

/// Menu that calls a function when selected
/// 
/// Ex.
/// ```
/// void func1(dictionary@ pArgs)
/// {
///    dprint("This function is called");
/// }
/// 
/// gui::menu_function func1_menu("func1", func1); /// Simple call. pArgs will be null.
/// ```
/// Ex.
/// ```
/// void func2(dictionary@ pArgs)
/// {
///    string@ msg = cast<string>(pArgs["msg"]);
///    dprint(msg);
/// }
/// gui::menu_function func2_menu("func2"
///    , func2
///    , dictionary = { { "msg", "This function called" } }); /// Call with arguments represented by a dictionary type.
/// ```
class menu_function : menu_item
{
	menu_function(const string&in pId, menu_function_t@ pCallback)
	{
		super(pId);
		@mCallback = pCallback;
	}
	
	menu_function(const string&in pId, menu_function_t@ pCallback, dictionary@ pParam)
	{
		super(pId);
		@mCallback = pCallback;
		@mParam = pParam;
	}
	
	menu@ open()
	{
		if (mCallback !is null)
		{
			mCallback(mParam);
		}
		mIs_selected = true;
		return @this;
	}
	
	private menu_function_t@ mCallback;
	private dictionary@ mParam;
};

/// Mixin for creating branches 
mixin class menu_branch
{
	/// Add a menu to the branch
	void add(menu@ pMenu) final
	{
		mSub_menus.insertLast(pMenu);
		menu_added(pMenu);
	}
	
	/// Remove a specific menu
	void remove(menu@ pMenu) final
	{
		for (uint i = 0; i < mSub_menus.length(); i++)
		{
			if (mSub_menus[i] is pMenu)
			{
				mSub_menus.removeAt(i);
				menu_removed(pMenu, i);
				return;
			}
		}
	}
	
	/// Remove all menus in branch
	void remove_all() final
	{
		mSub_menus.resize(0);
		menu_removed_all();
	}
	
	/// Get count of menus in branch
	uint count() final
	{
		return mSub_menus.length();
	}
	
	/// Get menu at index
	menu@ get_menu(uint pIndex) final
	{
		if (pIndex >= mSub_menus.length())
			return null;
		return mSub_menus[pIndex];
	}
	
	/// Callback when a new menu is added
	protected void menu_added(menu@ pMenu){ }
	
	/// Callback when a menu is deleted
	protected void menu_removed(menu@ pMenu, uint pIndex){ }
	
	/// Callback when all sub-menus are deleted
	protected void menu_removed_all(){ }
	
	private array<menu@> mSub_menus;
};

/// Selection menu for a list of text
class selection_menu : menu, menu_branch
{
	selection_menu()
	{
		mCan_return = false;
	}
	selection_menu(const string&in pId)
	{
		super(pId);
		mCan_return = true;
	}
	
	protected void menu_added(menu@ pMenu) 
	{
		mSelector.add(pMenu.get_id());
	}
	
	protected void menu_removed(menu@ pMenu, uint pIndex) 
	{
		mSelector.remove(pIndex);
	}
	
	protected void menu_removed_all() 
	{
		mSelector.remove_all();
	}
	
	/// Start selection
	menu@ open()
	{
		menu@ ret;
		do {
			int index = mSelector.start();
			if (index == -1)
			{
				if (mCan_return)
					return null;
				else continue;
			}
			
			@ret = get_menu(index).open();
		} while(ret is null);
		return ret;
	}
	
	/// Set_ position of list
	void set_position(vec pPosition)
	{
		mSelector.set_position(pPosition);
	}
	
	/// Set row limit. When the row limit is reached,
	/// the menu items will overflow to the next column and
	/// then the next column after that.
	void set_column_height(uint pRows)
	{
		mSelector.set_column_height(pRows);
	}
	
	/// Set whether of not the menu should be able to return
	/// to the menu before it without any selection.
	void set_return(bool pCan_return)
	{
		mCan_return = pCan_return;
	}
	
	private bool mCan_return;
	private basic_text_selection mSelector;
};


/// \}
}






