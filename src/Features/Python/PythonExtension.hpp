#pragma once

#include <Python.h>

PyObject* PyInit_game(void);

static PyObject* game_print(PyObject *self, PyObject *args);
static PyObject* game_execute_console(PyObject *self, PyObject *args);
static PyObject* game_entities(PyObject *self, PyObject *args);
static PyObject* game_entity_exists(PyObject *self, PyObject *args); // checks if handle valid
static PyObject* game_get_entity_name_by_handle(PyObject *self, PyObject *args);
static PyObject* game_get_entity_classname_by_handle(PyObject *self, PyObject *args);
static PyObject* game_create_entity_by_classname(PyObject *self, PyObject *args);
static PyObject* game_get_player(PyObject *self, PyObject *args);