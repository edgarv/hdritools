/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2013 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#if !defined( UTIL_H )
#define UTIL_H

#include <jni.h>
#include <string>
#include <Iex.h>


// Construct a Java Exception extending IExBase
DEFINE_EXC (JavaExc, Iex::BaseExc) 


// An utility function to "throw" java exceptions. When these methods are called
// the JVM is instructed to throw an exception but the C++ control flow does
// not change. Taken from:
//    http://java.sun.com/docs/books/jni/html/exceptions.html
//
void JNU_ThrowByName(JNIEnv *env, const char *name, const char *msg);

// Helper to throw a java.lang.IllegalArgumentException
inline void JNU_ThrowIllegalArgumentException(JNIEnv *env,
                                              const std::string& str) {
    JNU_ThrowByName(env, "java/lang/IllegalArgumentException", str.c_str());
}

// Helper to throw a java.lang.RuntimeException
inline void JNU_ThrowRuntimeException(JNIEnv *env, const std::string& str) {
    JNU_ThrowByName(env, "java/lang/RuntimeException", str.c_str());
}

// Get an environment from a cached JVM pointer
inline JNIEnv* getJNIEnv(JavaVM* jvm) {
    if (jvm != NULL) {
        union {
            JNIEnv* env;
            void* void_env;
        };
        jint retval = jvm->GetEnv(&void_env, JNI_VERSION_1_6);
        switch (retval) {
        case JNI_OK:
            return env;
            break;
        case JNI_EDETACHED:
            if (jvm->AttachCurrentThread(&void_env, NULL) != JNI_OK) {
                throw JavaExc("Could not get java environment for the thread");
            }
            return env;
            break;
        case JNI_EVERSION:
            throw JavaExc("Unsupported JNI version");
            break;
        default:
            throw JavaExc("Could not get the JVM thread environment");
        }
    } else {
        throw Iex::NullExc("null jvm pointer");
    }
}



/// Helper for strings which remain valid only during a single JNI Call
class JNIString
{
public:
    JNIString(JNIEnv* env, jstring javaString) : m_env(env),
        m_javaString(javaString), m_str(NULL), m_len(-1) {
        m_str = env->GetStringChars(javaString, NULL);
        m_len = env->GetStringLength(javaString);
    }

    ~JNIString() {
        if (m_str != NULL) {
            m_env->ReleaseStringChars(m_javaString, m_str);
            m_env->DeleteLocalRef(m_javaString);
        }
    }

    inline operator const jchar*() const {
        return m_str;
    }

    inline jint len() const {
        return m_len;
    }

private:
    JNIEnv* const m_env;
    jstring const m_javaString;
    const jchar* m_str;
    jint m_len;
};



/// Helper for UTF strings which remain valid only during a single JNI Call
class JNIUTFString
{
public:
    JNIUTFString(JNIEnv* env, jstring javaString) : m_env(env),
        m_javaString(javaString), m_str(NULL), m_len(-1) {
        m_str = env->GetStringUTFChars(javaString, NULL);
        m_len = env->GetStringUTFLength(javaString);
    }

    ~JNIUTFString() {
        if (m_str != NULL) {
            m_env->ReleaseStringUTFChars(m_javaString, m_str);
            m_env->DeleteLocalRef(m_javaString);
        }
    }

    inline operator const char*() const {
        return m_str;
    }

    inline jint len() const {
        return m_len;
    }

private:
    JNIEnv* const m_env;
    jstring const m_javaString;
    const char* m_str;
    jint m_len;
};



/// <summary>
/// Helper class which encapsulates a JNIEnv pointer so that some of its
/// commonly used functions throw a C++ exception in case of an error.
/// </summary>
class JNIEnvHelper
{
public:
    JNIEnvHelper(JNIEnv* env);

    /// <summary>
    /// Returns the Java VM interface (used in the Invocation API)
    /// associated with the current thread. Throws JavaExc in case of error.
    /// </summary>
    JavaVM* getJavaVM();

    /// <summary>
    /// Returns a global reference to a class object from a fully-qualified
    /// class name. Throws JavaExc in case of error.
    /// </summary>
    jclass findClassGlobalRef(const char* name);

    /// <summary>
    /// Returns a local reference to a class object from a fully-qualified
    /// class name. Throws JavaExc in case of error.
    /// </summary>
    jclass findClassLocalRef(const char* name);

    /// <summary>
    /// Returns the method ID handle for an instance (nonstatic) method of a
    /// class or interface. The method is specified by its name and signature.
    /// Throws JavaExc in case of error.
    /// </summary>
    jmethodID getMethodID(jclass clazz, const char* name, const char* sig);

    /// <summary>
    /// Returns the method ID handle for a static method of a class.
    /// The method is specified by its name and signature.
    /// Throws JavaExc in case of error.
    /// </summary>
    jmethodID getStaticMethodID(jclass clazz, const char* name, const char* sig);

    /// <summary>
    /// Returns the field ID handle for an instance (nonstatic) field of a 
    /// class. The field is specified by its name and signature.
    /// Throws JavaExc in case of error.
    /// </summary>
    jfieldID getFieldID(jclass clazz, const char* name, const char* sig);

private:
    JNIEnv* const m_env;
};



// C++11 Magic to write less code, for functions which return a value
template <class MethodImpl, typename T>
T safeCall(JNIEnv* env, const MethodImpl& method, const T& defaultValue)
{
    try {
        T retVal = method(env);
        return retVal;
    } catch (JavaExc& ex0) {
        if (env->ExceptionCheck() == JNI_FALSE) {
            std::ostringstream os;
            os << ex0.what();
            if (!ex0.stackTrace().empty()) {
                os << std::endl << "Stack trace:" 
                   << std::endl << ex0.stackTrace();
            }
            JNU_ThrowRuntimeException(env, os.str());
        }
    } catch (Iex::BaseExc& ex1) {
        std::ostringstream os;
        os << ex1.what();
        if (!ex1.stackTrace().empty()) {
            os << std::endl << "Stack trace:" << std::endl << ex1.stackTrace();
        }
        JNU_ThrowRuntimeException(env, os.str());
    } catch (std::exception& ex2) {
        JNU_ThrowRuntimeException(env, ex2.what());
    } catch (...) {
        JNU_ThrowRuntimeException(env, "Unknown native error!");
    }

    return defaultValue;
}



// C++11 Magic to write less code, for void functions
template <class MethodImpl>
void safeCall(JNIEnv* env, const MethodImpl& method)
{
    try {
        method(env);
    } catch (JavaExc& ex0) {
        if (env->ExceptionCheck() == JNI_FALSE) {
            std::ostringstream os;
            os << ex0.what();
            if (!ex0.stackTrace().empty()) {
                os << std::endl << "Stack trace:" 
                   << std::endl << ex0.stackTrace();
            }
            JNU_ThrowRuntimeException(env, os.str());
        }
    } catch (Iex::BaseExc& ex1) {
        std::ostringstream os;
        os << ex1.what();
        if (!ex1.stackTrace().empty()) {
            os << std::endl << "Stack trace:" << std::endl << ex1.stackTrace();
        }
        JNU_ThrowRuntimeException(env, os.str());
    } catch (std::exception& ex2) {
        JNU_ThrowRuntimeException(env, ex2.what());
    } catch (...) {
        JNU_ThrowRuntimeException(env, "Unknown native error!");
    }
}

#endif // UTIL_H
