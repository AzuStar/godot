# API ClassDB/Object Model

Native class basics:
- Root native class is `Object` (`core/object/object.h`), not explicitly derived.
- Ref-counted objects derive `RefCounted : Object` and use `Ref<T>`; manual lifetime objects derive `Object`/`Node` families.
- Serializable/loadable resources derive `Resource : RefCounted`; resource path/cache/local-to-scene behavior lives in `core/io/resource.h`.
- Scene tree classes derive `Node : Object`; GUI controls derive through `CanvasItem/Control`; most data assets derive through `Resource`.

Binding pattern:
- Headers use `GDCLASS(ClassName, BaseName)` inside class body.
- Exposed members bind in static `_bind_methods()` in `.cpp` using `ClassDB::bind_method(D_METHOD(...), ...)`.
- Properties use `ADD_PROPERTY(PropertyInfo(...), setter, getter)`; enums generally need `VARIANT_ENUM_CAST` in headers and constants/binds in `_bind_methods`.
- Virtual methods exported to scripts use `GDVIRTUAL*` macros.
- Resource base extensions use `RES_BASE_EXTENSION(...)`; saved native resource type may use `OBJ_SAVE_TYPE(...)`.

Docs/API sync:
- If exposed method/property/signal/enum changes, update matching `doc/classes/<Class>.xml`.
- XML class `inherits` should match native base from `GDCLASS`/C++ inheritance.

Known queried chain:
- `scene/resources/style_box.h`: `class StyleBox : public Resource`, `GDCLASS(StyleBox, Resource)`.
- `core/io/resource.h`: `class Resource : public RefCounted`.
- `core/object/ref_counted.h`: `class RefCounted : public Object`.
- Therefore `StyleBox -> Resource -> RefCounted -> Object`.