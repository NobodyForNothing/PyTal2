#include "PythonEntity.hpp"

#define PY_SSIZE_T_CLEAN
#include "Features/EntityList.hpp"
#include "Modules/Client.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Offsets.hpp"
#include "PytalMain.hpp"

#include <Python.h>

void Custom_dealloc(Entity *self) {
	Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject *Custom_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	Entity *self;
	self = (Entity *) type->tp_alloc(type, 0);
	return (PyObject *) self;
}
int Custom_init(Entity *self, PyObject *args, PyObject *kwds) {
	static char *kwlist[] = {(char *) "m_SerialNumber", nullptr};
	int serial = -1;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, serial))
		return -1;

}

// methods
PyObject *Entity_exists(Entity *self, PyObject *Py_UNUSED(ignored)) {
	// todo: check if it covers all cases
	if(self->entity != nullptr) return Py_True;
	return Py_False;
}
PyObject *Entity_get_name(Entity *self, PyObject *Py_UNUSED(ignored)) {
	char* entity_name = server->GetEntityName(self->entity); // TODO: does this work ?
	return PyUnicode_FromString(entity_name);
}
PyObject *Entity_classname(Entity *self, PyObject *Py_UNUSED(ignored)) {
	char* entity_name = server->GetEntityClassName(self->entity);
	if (entity_name == nullptr) return Py_None;
	return PyUnicode_FromString(entity_name);
}
PyObject *Entity_get_origin(Entity *self, PyObject *Py_UNUSED(ignored)) {
	PyObject *outputTuple = PyTuple_New(3);
	auto pos = server->GetAbsOrigin(self->entity);
	PyTuple_SetItem(outputTuple, 0, PyFloat_FromDouble(pos.x));
	PyTuple_SetItem(outputTuple, 1, PyFloat_FromDouble(pos.y));
	PyTuple_SetItem(outputTuple, 2, PyFloat_FromDouble(pos.z));
	return outputTuple; // TODO: check behavior in case the entity is not found
}
PyObject *Entity_get_angles(Entity *self, PyObject *Py_UNUSED(ignored)) {
	auto viewAngles = QAngle(); // todo: decide which one to get
	engine->GetViewAngles(self->entity, viewAngles); // same as cl_showpos
	// vscript does: server->GetAbsAngles(self->entity)

	PyObject *outputTuple = PyTuple_New(3);
	PyTuple_SetItem(outputTuple, 0, PyFloat_FromDouble(viewAngles.x));
	PyTuple_SetItem(outputTuple, 1, PyFloat_FromDouble(viewAngles.y));
	PyTuple_SetItem(outputTuple, 2, PyFloat_FromDouble(viewAngles.z));
	return outputTuple; // TODO: check behavior in case the entity is not found
}
PyObject *Entity_get_velocity(Entity *self, PyObject *Py_UNUSED(ignored)) {
	PyObject *outputTuple = PyTuple_New(3);
	auto localVelocity = server->GetLocalVelocity(self->entity);
	PyTuple_SetItem(outputTuple, 0, PyFloat_FromDouble(localVelocity.x));
	PyTuple_SetItem(outputTuple, 1, PyFloat_FromDouble(localVelocity.y));
	PyTuple_SetItem(outputTuple, 2, PyFloat_FromDouble(localVelocity.z));
	return outputTuple; // TODO: check behavior in case the entity is not found
}
PyObject *Entity_kill(Entity *self, PyObject *Py_UNUSED(ignored)) {
	if (self->entity != nullptr) {
		server->KillEntity(self->entity);
		self->entity = nullptr;
	}
	return Py_None;
}
PyObject *Entity_has_key(Entity *self, PyObject *args) {
}
PyObject *Entity_get_val(Entity *self, PyObject *args) {
}
PyObject *Entity_set_keyval(Entity *self, PyObject *args) {
}
PyObject *Entity_fire_input(Entity *self, PyObject *args) {
	void *entity = self->entity;
	char *input;
	void *activator;
	void *caller;
	variant_t val = {0};


	val.fieldType = FIELD_VOID;
	void *player = server->GetPlayer(1);
	server->AcceptInput(entity, "Kill", player, player, val, 0);

}
PyObject *Entity_on_output(Entity *self, PyObject *args) {
}