//
//     Copyright 2011, Kay Hayen, mailto:kayhayen@gmx.de
//
//     Part of "Nuitka", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     If you submit Kay Hayen patches to this software in either form, you
//     automatically grant him a copyright assignment to the code, or in the
//     alternative a BSD license to the code, should your jurisdiction prevent
//     this. Obviously it won't affect code that comes to him indirectly or
//     code you don't submit to him.
//
//     This is to reserve my ability to re-license the code at any time, e.g.
//     the PSF. With this version of Nuitka, using it for Closed Source will
//     not be allowed.
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, version 3 of the License.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//     Please leave the whole of this copyright notice intact.
//
#ifndef __NUITKA_EXCEPTIONS_H__
#define __NUITKA_EXCEPTIONS_H__

class _PythonException
{
    public:
        _PythonException()
        {
            this->line = _current_line;

            this->_importFromPython();
        }

        _PythonException( PyObject *exception )
        {
            assertObject( exception );

            this->line = _current_line;

            Py_INCREF( exception );

            this->exception_type = exception;
            this->exception_value = NULL;
            this->exception_tb = NULL;
        }

        _PythonException( PyObject *exception, PyTracebackObject *traceback )
        {
            assertObject( exception );
            assertObject( traceback );

            this->line = _current_line;

            this->exception_type = exception;
            this->exception_value = NULL;
            this->exception_tb = (PyObject *)traceback;
        }

        _PythonException( PyObject *exception, PyObject *value, PyTracebackObject *traceback )
        {
            assertObject( exception );
            assertObject( value );
            assertObject( traceback );

            this->line = _current_line;

            this->exception_type = exception;
            this->exception_value = value;
            this->exception_tb = (PyObject *)traceback;
        }

        _PythonException( const _PythonException &other )
        {
            this->line            = other.line;

            this->exception_type  = other.exception_type;
            this->exception_value = other.exception_value;
            this->exception_tb    = other.exception_tb;

            Py_XINCREF( this->exception_type );
            Py_XINCREF( this->exception_value );
            Py_XINCREF( this->exception_tb );
        }

        void operator=( const _PythonException &other )
        {
            Py_XINCREF( other.exception_type );
            Py_XINCREF( other.exception_value );
            Py_XINCREF( other.exception_tb );

            Py_XDECREF( this->exception_type );
            Py_XDECREF( this->exception_value );
            Py_XDECREF( this->exception_tb );

            this->line            = other.line;

            this->exception_type  = other.exception_type;
            this->exception_value = other.exception_value;
            this->exception_tb    = other.exception_tb;

        }

        ~_PythonException()
        {
            Py_XDECREF( this->exception_type );
            Py_XDECREF( this->exception_value );
            Py_XDECREF( this->exception_tb );
        }

        inline void _importFromPython()
        {
            PyErr_Fetch( &this->exception_type, &this->exception_value, &this->exception_tb );
            assert( this->exception_type );
        }

        inline int getLine() const
        {
            return this->line;
        }

        inline void normalize()
        {
            PyErr_NormalizeException( &this->exception_type, &this->exception_value, &this->exception_tb );
        }

        inline bool matches( PyObject *exception ) const
        {
            return PyErr_GivenExceptionMatches( this->exception_type, exception ) || PyErr_GivenExceptionMatches( this->exception_value, exception );;
        }

        inline void toPython()
        {
            PyErr_Restore( this->exception_type, this->exception_value, this->exception_tb );

#ifndef __NUITKA_NO_ASSERT__
            PyThreadState *thread_state = PyThreadState_GET();
#endif

            assert( this->exception_type == thread_state->curexc_type );
            assert( thread_state->curexc_type );

            this->exception_type  = NULL;
            this->exception_value = NULL;
            this->exception_tb    = NULL;
        }

        inline void toExceptionHandler()
        {
            this->normalize();

            // Restore only sets the current exception to the interpreter.
            PyThreadState *thread_state = PyThreadState_GET();

            PyObject *old_type  = thread_state->exc_type;
            PyObject *old_value = thread_state->exc_value;
            PyObject *old_tb    = thread_state->exc_traceback;

            thread_state->exc_type = this->exception_type;
            thread_state->exc_value = this->exception_value;
            thread_state->exc_traceback = this->exception_tb;

            Py_INCREF(  thread_state->exc_type );
            Py_XINCREF( thread_state->exc_value );
            Py_XINCREF(  thread_state->exc_traceback );

            Py_XDECREF( old_type );
            Py_XDECREF( old_value );
            Py_XDECREF( old_tb );

            PySys_SetObject( (char *)"exc_type", thread_state->exc_type );
            PySys_SetObject( (char *)"exc_value", thread_state->exc_value );
            PySys_SetObject( (char *)"exc_traceback", thread_state->exc_traceback );
        }

        inline PyObject *getType()
        {
            if ( this->exception_value == NULL )
            {
                PyErr_NormalizeException( &this->exception_type, &this->exception_value, &this->exception_tb );
            }

            return this->exception_type;
        }

        inline PyObject *getTraceback() const
        {
            return this->exception_tb;
        }

        inline void setTraceback( PyTracebackObject *traceback )
        {
            assert( traceback );
            assert( traceback->ob_refcnt > 0 );

            // printf( "setTraceback %d\n", traceback->ob_refcnt );

            // Py_INCREF( traceback );
            this->exception_tb = (PyObject *)traceback;
        }

        inline bool hasTraceback() const
        {
            return this->exception_tb != NULL;
        }

        void setType( PyObject *exception_type )
        {
            Py_XDECREF( this->exception_type );
            this->exception_type = exception_type;
        }

        inline PyObject *getObject()
        {
            PyErr_NormalizeException( &this->exception_type, &this->exception_value, &this->exception_tb );

            return this->exception_value;
        }

        void dump() const
        {
            PRINT_ITEMS( true, NULL, this->exception_type );
        }

    private:

        PyObject *exception_type, *exception_value, *exception_tb;
        int line;
};

class _PythonExceptionKeeper
{
    public:
        _PythonExceptionKeeper()
        {
            saved = NULL;
        }

        ~_PythonExceptionKeeper()
        {
            if ( this->saved )
            {
                delete this->saved;
            }
        }

        void save( const _PythonException &e )
        {
            this->saved = new _PythonException( e );
        }

        void rethrow()
        {
            if ( this->saved )
            {
                throw *this->saved;
            }
        }

        bool isEmpty() const
        {
            return this->saved == NULL;
        }

    private:
        bool empty;

        _PythonException *saved;
};

class ReturnException
{
};

class ContinueException
{
};

class BreakException
{
};

#endif