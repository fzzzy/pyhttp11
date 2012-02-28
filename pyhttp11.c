/*
*/

#include "Python.h"
#include "structmember.h"

#include "http11_parser.h"


typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyObject* environ;
    int header_done;
    http_parser *http;
} HttpParserObject;

static void
HttpParserObject_dealloc(HttpParserObject* self)
{
    Py_XDECREF(self->environ);
    free(self->http);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
HttpParserObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    HttpParserObject *self;

    self = (HttpParserObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->environ = PyDict_New();
        if (self->environ == NULL)
        {
            Py_DECREF(self);
            return NULL;
        }
        self->http = (http_parser*)malloc(sizeof(http_parser));
    }
    
    return (PyObject *)self;
}

void http_field_callback(void *data, const char *field_name, size_t field_name_len, const char *field_value, size_t field_value_size) {
    char field[field_name_len + 5];
    strncpy(field, "HTTP_", 5);
    strncpy(field + 5, field_name, field_name_len);

    PyDict_SetItem((PyObject *)data, PyString_FromStringAndSize(field, field_name_len + 5), PyString_FromStringAndSize(field_value, field_value_size));
}

void request_uri_callback(void *data, const char * buffer, size_t buffer_len) {
    PyDict_SetItem((PyObject *)data, PyString_FromFormat("REQUEST_URI"), PyString_FromStringAndSize(buffer, buffer_len));
}

void fragment_callback(void *data, const char * buffer, size_t buffer_len) {
    PyDict_SetItem((PyObject *)data, PyString_FromFormat("FRAGMENT"), PyString_FromStringAndSize(buffer, buffer_len));
}

void request_path_callback(void *data, const char * buffer, size_t buffer_len) {
    PyDict_SetItem((PyObject *)data, PyString_FromFormat("PATH_INFO"), PyString_FromStringAndSize(buffer, buffer_len));
}

void query_string_callback(void *data, const char * buffer, size_t buffer_len) {
    PyDict_SetItem((PyObject *)data, PyString_FromFormat("QUERY_STRING"), PyString_FromStringAndSize(buffer, buffer_len));
}

void http_version_callback(void *data, const char * buffer, size_t buffer_len) {
    PyDict_SetItem((PyObject *)data, PyString_FromFormat("HTTP_VERSION"), PyString_FromStringAndSize(buffer, buffer_len));

}

void request_method_callback(void *data, const char * buffer, size_t buffer_len) {
    PyDict_SetItem((PyObject *)data, PyString_FromFormat("REQUEST_METHOD"), PyString_FromStringAndSize(buffer, buffer_len));
}

void header_done_callback(void *data, const char * buffer, size_t buffer_len) {
    // buffer is body
    PyDict_SetItem((PyObject *)data, PyString_FromFormat("REQUEST_BODY"), PyString_FromStringAndSize(buffer, buffer_len));
}

static int
HttpParserObject_init(HttpParserObject *self, PyObject *args, PyObject *kwds)
{    
    self->http->http_field = http_field_callback;
    self->http->request_uri = request_uri_callback;
    self->http->fragment = fragment_callback;
    self->http->request_path = request_path_callback;
    self->http->query_string = query_string_callback;
    self->http->http_version = http_version_callback;
    self->http->header_done = header_done_callback;
    self->http->request_method = request_method_callback;
    http_parser_init(self->http);
    return 0;
}

static PyMemberDef HttpParserObject_members[] = {
    {"environ", T_OBJECT_EX, offsetof(HttpParserObject, environ), 0, ""},
    {NULL}  /* Sentinel */
};

static PyObject *
HttpParserObject_execute(HttpParserObject* self, PyObject* args)
{
    const char *data = NULL;
    size_t data_len = 0;

    if (!PyArg_ParseTuple(args, "s#", &data, &data_len))
        return NULL;

    self->http->data = self->environ;
    http_parser_execute(self->http, data, data_len, http_parser_nread(self->http));

    return PyInt_FromLong(http_parser_nread(self->http));
}

static PyObject *
HttpParserObject_has_error(HttpParserObject* self)
{
    if (http_parser_has_error(self->http)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject *
HttpParserObject_is_finished(HttpParserObject* self)
{    
    if (http_parser_is_finished(self->http)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject *
HttpParserObject_reset(HttpParserObject* self)
{
    Py_DECREF(self->environ);
    self->environ = PyDict_New();
    http_parser_init(self->http);
    Py_RETURN_NONE;
}

static PyMethodDef HttpParserObject_methods[] = {
    {"execute", (PyCFunction)HttpParserObject_execute, METH_VARARGS, ""},
    {"has_error", (PyCFunction)HttpParserObject_has_error, METH_NOARGS, ""},
    {"is_finished", (PyCFunction)HttpParserObject_is_finished, METH_NOARGS, ""},
    {"reset", (PyCFunction)HttpParserObject_reset, METH_VARARGS, ""},
    {NULL}  /* Sentinel */
};

static PyTypeObject HttpParserObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyhttp11.HttpParser",             /*tp_name*/
    sizeof(HttpParserObjectType), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)HttpParserObject_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "An http parser created by wrapping mongrel's http11 for Python.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    HttpParserObject_methods,             /* tp_methods */
    HttpParserObject_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)HttpParserObject_init,      /* tp_init */
    0,                         /* tp_alloc */
    HttpParserObject_new,                 /* tp_new */
};

static PyMethodDef Nothing[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initpyhttp11(void)
{
    PyObject* m;
    
    if (PyType_Ready(&HttpParserObjectType) < 0)
        return;

    m = Py_InitModule("pyhttp11", Nothing);

    Py_INCREF(&HttpParserObjectType);
    PyModule_AddObject(m, "HttpParser", (PyObject *)&HttpParserObjectType);
}
