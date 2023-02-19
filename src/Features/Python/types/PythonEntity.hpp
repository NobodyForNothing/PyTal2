#pragma once

#include "Utils/SDK/EntityEdict.hpp"

#include <Python.h>

typedef struct Entity {
	// identifying entity by serial number,
	PyObject_HEAD
		void *entity;
} Entity;

void Custom_dealloc(Entity *self);
PyObject *Custom_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Custom_init(Entity *self, PyObject *args, PyObject *kwds);


PyObject *Entity_exists(Entity *self, PyObject *Py_UNUSED(ignored));
PyObject *Entity_get_name(Entity *self, PyObject *Py_UNUSED(ignored));
PyObject *Entity_classname(Entity *self, PyObject *Py_UNUSED(ignored));
PyObject *Entity_get_origin(Entity *self, PyObject *Py_UNUSED(ignored));
PyObject *Entity_get_angles(Entity *self, PyObject *Py_UNUSED(ignored));
PyObject *Entity_get_velocity(Entity *self, PyObject *Py_UNUSED(ignored));
PyObject *Entity_kill(Entity *self, PyObject *Py_UNUSED(ignored));
PyObject *Entity_has_key(Entity *self, PyObject *args);
PyObject *Entity_get_val(Entity *self, PyObject *args);
PyObject *Entity_set_keyval(Entity *self, PyObject *args);
PyObject *Entity_fire_input(Entity *self, PyObject *args);
PyObject *Entity_on_output(Entity *self, PyObject *args);

//todo differential local and global pos, ang, vel
// todo eyepos
static PyMethodDef Entity_methods[] = {
	{"exists", (PyCFunction) Entity_exists, METH_NOARGS,
  "Returns true if the referenced entity still exists, otherwise false"
 },
	{"name", (PyCFunction) Entity_get_name, METH_NOARGS,
  "Returns the entity name or an empty string in the case of an error"
 },
	{"classname", (PyCFunction) Entity_classname, METH_NOARGS,
  "Returns the entity classname or None in the case of an error"
 },
	{"origin", (PyCFunction)Entity_get_origin, METH_NOARGS,
  "Returns origin as a float tuple [x, y, z]"
 },
	{"angles", (PyCFunction) Entity_get_angles, METH_NOARGS,
  "Returns angles as a float tuple [x, y, z]"
 },
	{"velocity", (PyCFunction) Entity_get_velocity, METH_NOARGS,
  "Returns velocity as a tuple [x, y, z]"
 },
	{"kill", (PyCFunction) Entity_kill, METH_NOARGS,
  "Removes entity from game."
 },
	{"has_key", (PyCFunction) Entity_has_key, METH_VARARGS,
  "Returns true if the referenced entity key-value-pairs contain a key, otherwise false"
 },
	{"get_val", (PyCFunction) Entity_get_val, METH_VARARGS,
  "Reads out the value associated with a key. If the key is not found a empty string is returned."
 },
	{"set_keyval", (PyCFunction) Entity_set_keyval, METH_VARARGS,
  "Sets a key-value pair of the entity returns bool indicating success."
 },
	{"fire_input", (PyCFunction) Entity_fire_input, METH_VARARGS,
  "Fires entity output returns bool indicating success."
 },
	{"on_output", (PyCFunction) Entity_on_output, METH_VARARGS,
  "Attaches a function call to a entity input. The function will be executed with the input name as the only argument everytime the input is fired."
 },
	{nullptr}
};
static PyTypeObject EntityType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
		.tp_name = "game.Entity",
	.tp_basicsize = sizeof(Entity),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = PyDoc_STR("Access and modify a single entity"),
	.tp_methods = Entity_methods,
	.tp_init = reinterpret_cast<initproc>(Custom_init),
	.tp_new = Custom_new,
};

