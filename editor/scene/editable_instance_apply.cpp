/**************************************************************************/
/*  editable_instance_apply.cpp                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editable_instance_apply.h"

#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_toaster.h"
#include "editor/scene/scene_tree_editor.h"
#include "scene/property_utils.h"
#include "scene/resources/packed_scene.h"

struct EditableInstanceApplyEntry {
	Node *node = nullptr;
	StringName property;
	PropertyInfo property_info;
};

static bool _get_property_info(const Node *p_node, const StringName &p_property, PropertyInfo *r_property_info = nullptr) {
	ERR_FAIL_NULL_V(p_node, false);

	List<PropertyInfo> properties;
	p_node->get_property_list(&properties);
	for (const PropertyInfo &property_info : properties) {
		if (property_info.name == p_property) {
			if (r_property_info) {
				*r_property_info = property_info;
			}
			return true;
		}
	}

	return false;
}

static bool _is_property_applicable(const Node *p_node, const StringName &p_property, PropertyInfo *r_property_info = nullptr) {
	PropertyInfo property_info;
	if (!_get_property_info(p_node, p_property, &property_info)) {
		return false;
	}

	if (!(property_info.usage & PROPERTY_USAGE_STORAGE)) {
		return false;
	}
	if (property_info.usage & PROPERTY_USAGE_READ_ONLY) {
		return false;
	}

	if (property_info.usage & PROPERTY_USAGE_NO_INSTANCE_STATE) {
		Vector<SceneState::PackState> states_stack = PropertyUtils::get_node_states_stack(p_node, EditorNode::get_singleton()->get_edited_scene());
		if (!states_stack.is_empty()) {
			return false;
		}
	}

	if (r_property_info) {
		*r_property_info = property_info;
	}
	return true;
}

static bool _is_node_in_instance(Node *p_node, Node *p_instance_root) {
	return p_node == p_instance_root || p_instance_root->is_ancestor_of(p_node);
}

static bool _remap_node_to_source(Node *p_node, Node *p_instance_root, Node *p_source_root, Node **r_source_node) {
	ERR_FAIL_NULL_V(r_source_node, false);
	*r_source_node = nullptr;

	if (!p_node) {
		return true;
	}
	if (!_is_node_in_instance(p_node, p_instance_root)) {
		return false;
	}
	if (p_node == p_instance_root) {
		*r_source_node = p_source_root;
		return true;
	}

	NodePath path = p_instance_root->get_path_to(p_node);
	Node *source_node = p_source_root->get_node_or_null(path);
	if (!source_node) {
		return false;
	}

	*r_source_node = source_node;
	return true;
}

static bool _is_node_object_hint(const String &p_subtype_string) {
	int slash_pos = p_subtype_string.find_char('/');
	PropertyHint subtype_hint = PROPERTY_HINT_NONE;
	String subtype_string = p_subtype_string;

	if (slash_pos >= 0) {
		subtype_hint = PropertyHint(subtype_string.get_slicec('/', 1).to_int());
		subtype_string = subtype_string.substr(0, slash_pos);
	}

	return Variant::Type(subtype_string.to_int()) == Variant::OBJECT && subtype_hint == PROPERTY_HINT_NODE_TYPE;
}

static bool _get_array_node_object_hint(const PropertyInfo &p_property_info) {
	if (p_property_info.type != Variant::ARRAY || p_property_info.hint != PROPERTY_HINT_TYPE_STRING) {
		return false;
	}

	const int hint_subtype_separator = p_property_info.hint_string.find_char(':');
	if (hint_subtype_separator < 0) {
		return false;
	}

	return _is_node_object_hint(p_property_info.hint_string.substr(0, hint_subtype_separator));
}

static void _get_dictionary_node_object_hints(const PropertyInfo &p_property_info, bool *r_key_is_node, bool *r_value_is_node) {
	*r_key_is_node = false;
	*r_value_is_node = false;

	if (p_property_info.type != Variant::DICTIONARY || p_property_info.hint != PROPERTY_HINT_TYPE_STRING) {
		return;
	}

	const int key_value_separator = p_property_info.hint_string.find_char(';');
	if (key_value_separator < 0) {
		return;
	}

	const int key_subtype_separator = p_property_info.hint_string.find_char(':');
	if (key_subtype_separator < 0 || key_subtype_separator > key_value_separator) {
		return;
	}

	const String key_subtype_string = p_property_info.hint_string.substr(0, key_subtype_separator);
	const int value_subtype_separator = p_property_info.hint_string.find_char(':', key_value_separator);
	if (value_subtype_separator < 0) {
		return;
	}

	const String value_subtype_string = p_property_info.hint_string.substr(key_value_separator + 1, value_subtype_separator - (key_value_separator + 1));
	*r_key_is_node = _is_node_object_hint(key_subtype_string);
	*r_value_is_node = _is_node_object_hint(value_subtype_string);
}

static bool _duplicate_builtin_resources(const Variant &p_value, Variant *r_value) {
	switch (p_value.get_type()) {
		case Variant::OBJECT: {
			Ref<Resource> resource = p_value;
			if (resource.is_valid() && resource->is_built_in()) {
				*r_value = resource->duplicate(true);
			} else {
				*r_value = p_value;
			}
			return true;
		}
		case Variant::ARRAY: {
			Array source_array = p_value;
			Array target_array;
			target_array.resize(source_array.size());
			for (int i = 0; i < source_array.size(); i++) {
				Variant value;
				if (!_duplicate_builtin_resources(source_array[i], &value)) {
					return false;
				}
				target_array[i] = value;
			}
			*r_value = target_array;
			return true;
		}
		case Variant::DICTIONARY: {
			Dictionary source_dictionary = p_value;
			Dictionary target_dictionary;
			for (const KeyValue<Variant, Variant> &kv : source_dictionary) {
				Variant key;
				Variant value;
				if (!_duplicate_builtin_resources(kv.key, &key) || !_duplicate_builtin_resources(kv.value, &value)) {
					return false;
				}
				target_dictionary[key] = value;
			}
			*r_value = target_dictionary;
			return true;
		}
		default: {
			*r_value = p_value;
			return true;
		}
	}
}

static bool _remap_node_value(const Variant &p_value, Node *p_instance_root, Node *p_source_root, Variant *r_value) {
	Node *node = Object::cast_to<Node>(p_value);
	Node *source_node = nullptr;
	if (!_remap_node_to_source(node, p_instance_root, p_source_root, &source_node)) {
		return false;
	}
	*r_value = source_node;
	return true;
}

static bool _prepare_property_value(const PropertyInfo &p_property_info, const Variant &p_value, Node *p_instance_root, Node *p_source_root, Variant *r_value) {
	if (p_property_info.type == Variant::OBJECT && p_property_info.hint == PROPERTY_HINT_NODE_TYPE) {
		return _remap_node_value(p_value, p_instance_root, p_source_root, r_value);
	}

	if (_get_array_node_object_hint(p_property_info)) {
		Array source_array = p_value;
		Array target_array;
		target_array.resize(source_array.size());
		for (int i = 0; i < source_array.size(); i++) {
			Variant value;
			if (!_remap_node_value(source_array[i], p_instance_root, p_source_root, &value)) {
				return false;
			}
			target_array[i] = value;
		}
		*r_value = target_array;
		return _duplicate_builtin_resources(*r_value, r_value);
	}

	bool key_is_node = false;
	bool value_is_node = false;
	_get_dictionary_node_object_hints(p_property_info, &key_is_node, &value_is_node);
	if (key_is_node || value_is_node) {
		Dictionary source_dictionary = p_value;
		Dictionary target_dictionary;
		for (const KeyValue<Variant, Variant> &kv : source_dictionary) {
			Variant key = kv.key;
			Variant value = kv.value;
			if (key_is_node && !_remap_node_value(key, p_instance_root, p_source_root, &key)) {
				return false;
			}
			if (value_is_node && !_remap_node_value(value, p_instance_root, p_source_root, &value)) {
				return false;
			}
			target_dictionary[key] = value;
		}
		*r_value = target_dictionary;
		return _duplicate_builtin_resources(*r_value, r_value);
	}

	return _duplicate_builtin_resources(p_value, r_value);
}

static Node *_get_source_node(Node *p_node, Node *p_instance_root, Node *p_source_root) {
	if (p_node == p_instance_root) {
		return p_source_root;
	}

	return p_source_root->get_node_or_null(p_instance_root->get_path_to(p_node));
}

static void _collect_property_overrides(Node *p_node, Node *p_instance_root, Vector<EditableInstanceApplyEntry> &r_entries) {
	if (!p_node || p_node->get_owner() != p_instance_root) {
		return;
	}

	List<PropertyInfo> properties;
	p_node->get_property_list(&properties);
	for (const PropertyInfo &property_info : properties) {
		if (!(property_info.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}
		if (property_info.usage & PROPERTY_USAGE_READ_ONLY) {
			continue;
		}
		if ((property_info.usage & PROPERTY_USAGE_NO_INSTANCE_STATE) && !PropertyUtils::get_node_states_stack(p_node, EditorNode::get_singleton()->get_edited_scene()).is_empty()) {
			continue;
		}
		if (!EditableInstanceApply::has_property_override(p_node, property_info.name)) {
			continue;
		}

		EditableInstanceApplyEntry entry;
		entry.node = p_node;
		entry.property = property_info.name;
		entry.property_info = property_info;
		r_entries.push_back(entry);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		if (child->get_owner() == p_instance_root) {
			_collect_property_overrides(child, p_instance_root, r_entries);
		}
	}
}

static Error _refresh_instance_state(Node *p_source_root, Node *p_instance_root, const String &p_source_path, String *r_error_message) {
	Ref<PackedScene> packed_scene = ResourceCache::get_ref(p_source_path);
	if (packed_scene.is_null()) {
		packed_scene.instantiate();
		packed_scene->set_path(p_source_path, true);
	}

	Error err = packed_scene->pack(p_source_root);
	if (err != OK) {
		if (r_error_message) {
			*r_error_message = vformat("Failed to pack source scene '%s'.", p_source_path);
		}
		return err;
	}

	packed_scene->set_path(p_source_path, true);
	p_instance_root->set_scene_instance_state(packed_scene->get_state());
	return OK;
}

static Node *_get_or_open_source_scene(const String &p_source_path, int *r_source_scene_idx, int p_edited_scene_idx, bool *r_opened_scene, Error *r_error) {
	EditorNode *editor = EditorNode::get_singleton();
	EditorData &editor_data = EditorNode::get_editor_data();

	*r_opened_scene = false;
	*r_source_scene_idx = editor_data.get_edited_scene_from_path(p_source_path);
	if (*r_source_scene_idx < 0) {
		*r_error = editor->load_scene(p_source_path, false, false, false, true);
		if (*r_error != OK) {
			return nullptr;
		}
		*r_opened_scene = true;
		*r_source_scene_idx = editor_data.get_edited_scene_from_path(p_source_path);
	}

	if (*r_source_scene_idx < 0) {
		*r_error = ERR_FILE_NOT_FOUND;
		return nullptr;
	}

	Node *source_root = editor_data.get_edited_scene_root(*r_source_scene_idx);
	if (!source_root) {
		*r_error = ERR_CANT_OPEN;
		return nullptr;
	}

	if (*r_opened_scene && p_edited_scene_idx >= 0 && editor_data.get_edited_scene() != p_edited_scene_idx) {
		EditableInstanceApply::_restore_edited_scene(p_edited_scene_idx);
	}

	*r_error = OK;
	return source_root;
}

static EditableInstanceApply::ApplyResult _apply_entries(Node *p_instance_root, const Vector<EditableInstanceApplyEntry> &p_entries) {
	EditableInstanceApply::ApplyResult result;
	if (p_entries.is_empty()) {
		return result;
	}

	const String source_path = p_instance_root->get_scene_file_path();
	if (source_path.is_empty()) {
		result.error = ERR_FILE_NOT_FOUND;
		result.message = "Editable instance source path is empty.";
		return result;
	}

	EditorData &editor_data = EditorNode::get_editor_data();
	const int edited_scene_idx = editor_data.get_edited_scene();

	int source_scene_idx = -1;
	bool opened_scene = false;
	Error err = OK;
	Node *source_root = _get_or_open_source_scene(source_path, &source_scene_idx, edited_scene_idx, &opened_scene, &err);
	if (!source_root) {
		result.error = err;
		result.message = vformat("Failed to open source scene '%s'.", source_path);
		return result;
	}

	for (const EditableInstanceApplyEntry &entry : p_entries) {
		Node *source_node = _get_source_node(entry.node, p_instance_root, source_root);
		if (!source_node) {
			result.skipped_properties++;
			WARN_PRINT(vformat("Cannot apply '%s' to original: source node '%s' does not exist.", entry.property, String(p_instance_root->get_path_to(entry.node))));
			continue;
		}

		PropertyInfo source_property_info;
		if (!_is_property_applicable(source_node, entry.property, &source_property_info)) {
			result.skipped_properties++;
			WARN_PRINT(vformat("Cannot apply '%s' to original: source property does not exist or is not stored.", entry.property));
			continue;
		}

		Variant value = entry.node->get(entry.property);
		Variant prepared_value;
		if (!_prepare_property_value(entry.property_info, value, p_instance_root, source_root, &prepared_value)) {
			result.skipped_properties++;
			WARN_PRINT(vformat("Cannot apply '%s' to original: node reference points outside editable instance.", entry.property));
			continue;
		}

		source_node->set(entry.property, prepared_value);
		result.applied_properties++;
	}

	if (result.applied_properties == 0) {
		if (result.skipped_properties > 0) {
			result.error = ERR_CANT_CREATE;
			result.message = vformat("%d properties could not be applied to original.", result.skipped_properties);
		}
		return result;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (source_scene_idx >= 0) {
		undo_redo->set_history_as_unsaved(editor_data.get_scene_history_id(source_scene_idx));
	}

	err = _refresh_instance_state(source_root, p_instance_root, source_path, &result.message);
	if (err != OK) {
		result.error = err;
		undo_redo->emit_signal(SNAME("history_changed"));
		return result;
	}

	if (edited_scene_idx >= 0) {
		undo_redo->set_history_as_unsaved(editor_data.get_scene_history_id(edited_scene_idx));
	}
	undo_redo->emit_signal(SNAME("history_changed"));

	SceneTreeDock::get_singleton()->get_tree_editor()->update_tree();
	InspectorDock::get_inspector_singleton()->update_tree();

	if (result.skipped_properties > 0) {
		EditorToaster::get_singleton()->popup_str(vformat(TTR("%d properties applied to original. %d properties skipped."), result.applied_properties, result.skipped_properties), EditorToaster::SEVERITY_WARNING);
	} else {
		EditorToaster::get_singleton()->popup_str(vformat(TTR("%d properties applied to original."), result.applied_properties), EditorToaster::SEVERITY_INFO);
	}

	return result;
}

void EditableInstanceApply::_restore_edited_scene(int p_scene_idx) {
	EditorNode::get_singleton()->_set_current_scene(p_scene_idx);
}

bool EditableInstanceApply::is_editable_child(const Node *p_node, Node **r_instance_root, Node **r_edited_scene) {
	if (r_instance_root) {
		*r_instance_root = nullptr;
	}
	if (r_edited_scene) {
		*r_edited_scene = nullptr;
	}

	if (!p_node || !EditorNode::get_singleton()) {
		return false;
	}

	Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();
	if (!edited_scene || p_node == edited_scene || !p_node->get_owner()) {
		return false;
	}

	Node *owner = p_node->get_owner();
	if (owner == edited_scene || !edited_scene->is_editable_instance(owner)) {
		return false;
	}

	if (r_instance_root) {
		*r_instance_root = owner;
	}
	if (r_edited_scene) {
		*r_edited_scene = edited_scene;
	}
	return true;
}

bool EditableInstanceApply::can_show_apply_to_original(const Node *p_node, const StringName &p_property) {
	if (!is_editable_child(p_node)) {
		return false;
	}

	return _is_property_applicable(p_node, p_property);
}

bool EditableInstanceApply::has_property_override(const Node *p_node, const StringName &p_property) {
	Node *instance_root = nullptr;
	Node *edited_scene = nullptr;
	if (!is_editable_child(p_node, &instance_root, &edited_scene)) {
		return false;
	}

	PropertyInfo property_info;
	if (!_is_property_applicable(p_node, p_property, &property_info)) {
		return false;
	}

	Vector<SceneState::PackState> states_stack = PropertyUtils::get_node_states_stack(p_node, edited_scene);
	bool is_valid_default = false;
	Variant default_value = PropertyUtils::get_property_default_value(p_node, p_property, &is_valid_default, &states_stack);
	if (!is_valid_default) {
		return false;
	}

	bool valid = false;
	Variant current_value = p_node->get(p_property, &valid);
	if (!valid) {
		return false;
	}

	return PropertyUtils::is_property_value_different(p_node, current_value, default_value);
}

int EditableInstanceApply::count_overrides_in_subtree(const Node *p_node) {
	Node *instance_root = nullptr;
	if (!is_editable_child(p_node, &instance_root)) {
		return 0;
	}

	Vector<EditableInstanceApplyEntry> entries;
	_collect_property_overrides(const_cast<Node *>(p_node), instance_root, entries);
	return entries.size();
}

EditableInstanceApply::ApplyResult EditableInstanceApply::apply_property(Node *p_node, const StringName &p_property) {
	ApplyResult result;
	Node *instance_root = nullptr;
	Node *edited_scene = nullptr;
	if (!is_editable_child(p_node, &instance_root, &edited_scene)) {
		result.error = ERR_INVALID_PARAMETER;
		result.message = "Node is not an editable child.";
		return result;
	}

	if (!has_property_override(p_node, p_property)) {
		return result;
	}

	PropertyInfo property_info;
	if (!_is_property_applicable(p_node, p_property, &property_info)) {
		result.error = ERR_INVALID_PARAMETER;
		result.message = "Property cannot be applied to original.";
		return result;
	}

	EditableInstanceApplyEntry entry;
	entry.node = p_node;
	entry.property = p_property;
	entry.property_info = property_info;

	Vector<EditableInstanceApplyEntry> entries;
	entries.push_back(entry);
	return _apply_entries(instance_root, entries);
}

EditableInstanceApply::ApplyResult EditableInstanceApply::apply_subtree(Node *p_node) {
	ApplyResult result;
	Node *instance_root = nullptr;
	Node *edited_scene = nullptr;
	if (!is_editable_child(p_node, &instance_root, &edited_scene)) {
		result.error = ERR_INVALID_PARAMETER;
		result.message = "Node is not an editable child.";
		return result;
	}

	Vector<EditableInstanceApplyEntry> entries;
	_collect_property_overrides(p_node, instance_root, entries);
	return _apply_entries(instance_root, entries);
}
