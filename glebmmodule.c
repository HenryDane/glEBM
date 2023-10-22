#define PY_SSIZE_T_CLEAN
#include <Python.h>

/*
    glebm.init(size_x: int, size_y: int, 
        Ts: np.ndarray = None,
        qs: np.ndarray = None)
    Initializes the glebm instance. If the model has already been initialized,
    this will raise an exception.
    
    size_x and size_y must be powers of two and greater than 32.
    
    If Ts is not none, Ts.shape must equal (size_x, size_y).
    The above applies for qs as well.
*/ 

static PyObject* glebm_init(PyObject* self, PyObject* args) {
    int size_x, size_y;
    
    // try to read arguments
    if (!PyArg_ParseTuple(args, "i|i", &size_x, &size_y)) {
        return NULL;
    }
    
    // check size_x is mult of 32
    if (size_x % 32 != 0) {
        PyErr_SetString(PyExc_ValueError, "size_x must be a multiple of 32");
        return NULL;
    }
    
    // check size_y is mult of 32
    if (size_y % 32 != 0) {
        PyErr_SetString(PyExc_ValueError, "size_y must be a multiple of 32");
        return NULL;
    }
    
    printf("%d %d\n", size_x, size_y);
    
    Py_RETURN_NONE;
}

/*
    glebm.reset() -> bool
    This will reset the model (equivalent to running init) except that it will
    return false if the model was never initialized beforehand OR if there
    was a problem resetting the model.
*/
static PyObject* glebm_reset(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

/* 
    glebm.launch() -> bool
    This will launch the model IN A SEPERATE THREAD (and will create a window)  
    Returns true if launch was successful, false otherwise.
*/
static PyObject* glebm_launch(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

/*
    glebm.abort()
    This will stop any running model (and may block until it has stopped it 
    successfully).
*/
static PyObject* glebm_abort(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

/*
    glebm.block_until(t: float) -> dict[str, np.ndarray]
    This will block until the model reaches some time t, at which point it will
    return the state of the model as a dictionary.
*/
static PyObject* glebm_block_until(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

/*
    glebm.block()
    This will block until the model finishes running.
*/
static PyObject* glebm_block(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

/*
    glebm.state() -> dict[str, np.ndarray]
    This will return the state of the model. If the model is running, it may 
    block until the state can be read. If the model is not running it will 
    return the last known state.
*/
static PyObject* glebm_state(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

static PyMethodDef glEBMMethods[] = {
    {"init", glebm_init, METH_VARARGS, "Initialize the model."},
    {"reset", glebm_reset, METH_VARARGS, "Reset the model."},
    {"launch", glebm_launch, METH_VARARGS, "Launch the model."},
    {"abort", glebm_abort, METH_VARARGS, "Abort a running model."},
    {"block_until", glebm_block_until, METH_VARARGS, 
        "Block until the model reaches a certian time."},
    {"block", glebm_block, METH_VARARGS, 
        "Block until the currently running model finishes."},
    {"state", glebm_state, METH_VARARGS, "Fetch state information."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef glEBM_module = {
    PyModuleDef_HEAD_INIT,
    "glebm",
    NULL, // no documentation
    -1,
    glEBMMethods
};

PyMODINIT_FUNC PyInit_glebm(void) {    
    // TODO: create some sort of context and initialize shaders/textures/etc.
    // TODO: figure out how and where to store internal state data. We need 
    // a way to keep track of our own internals without exposing them to 
    // the python system afaik.
    return PyModule_Create(&glEBM_module);  
}

