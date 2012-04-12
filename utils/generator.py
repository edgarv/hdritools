# Helper functions to generate a simple C++ wrapper to support
# enumeration in TCLAP


def jHash(s):
    '''Compute the hash of the given string alla Java, but using uint32.
See http://www.coderanch.com/t/329128/java/java/Java-String-hashcode-base-computation'''
    h = 0
    for c in s:
        h = 0xFFFFFFFF & ((0xFFFFFFFF & (31 * h)) + ord(c))
    return h
    


class generator(object):
    '''Super generator class from a magic template'''

    __template = r'''
class {classname}
{{
public:
    {classname}({enumtype} v = {enumdefault}) :
    m_value(v)
    {{}}

    {classname}(const std::string &str) : m_value(INVALID)
    {{
        std::string s(str);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        const uint32_t hash = jHash(s);
        switch (hash) {{
{string_to_enum_case}
        }}
    }}

    inline operator {enumtype}() const {{
        assert(m_value != INVALID);
        return static_cast<{enumtype}>(m_value);
    }}

    operator const char*() const {{
        switch (m_value) {{
{enum_to_string_case}
        default:
            return "unknown";
        }}
    }}

    inline bool operator== (const {classname} &v) const {{
        return m_value == v.m_value;
    }}

    inline bool operator!= (const {classname} &v) const {{
        return m_value != v.m_value;
    }}

    inline bool operator< (const {classname} &v) const {{
        return m_value < v.m_value;
    }}

    inline bool operator> (const {classname} &v) const {{
        return m_value > v.m_value;
    }}

    inline bool operator<= (const {classname} &v) const {{
        return m_value <= v.m_value;
    }}

    inline bool operator>= (const {classname} &v) const {{
        return m_value >= v.m_value;
    }}

    static const std::vector<{classname}>& values() {{
        return VALUES;
    }}


private:

    const static int INVALID = 0x7FFFFFFF;
    const static std::vector<{classname}> VALUES;

    static std::vector<{classname}> allValues()
    {{
         // C++11 Initializer lists would make this much easier
        std::vector<{classname}> vec;
{pushback_all_values}
        return vec;
    }}

    friend std::istream& operator>> (std::istream &is, {classname} &v)
    {{
        std::string val;
        is >> val;
        {classname} tmp(val);
        v.m_value = tmp.m_value;
        return is;
    }}

    friend std::ostream& operator<< (std::ostream &os, {classname} &v)
    {{
        const char* val = static_cast<const char*>(v);
        os << val;
        return os;
    }}

    int m_value;
}};

const std::vector<{classname}> {classname}::VALUES = {classname}::allValues();

template <>
struct TCLAP::ArgTraits<{classname}>
{{
    typedef ValueLike ValueCategory;
}};
'''

    def __init__(self, classname, typename, enum_default, enum_dict):
        '''Initialize with the classname of the generated type, the typename
of the enum type, the default enumeration value and the mapping between the
enumeration values and their string version.'''
        self.classname    = classname
        self.typename     = typename
        self.enum_default = enum_default
        self.enum_dict    = enum_dict


    def __string_to_enum_case(self):
        '''Generate the formatted case+if sequence for converting to enum.'''
        t = '''        case {0:#010x}:
            if (s == "{1}") m_value = {2};
            break;'''
        lst = [t.format(jHash(v.lower()), v.lower(), k) for k,v
               in self.enum_dict.items()]
        return '\n'.join(lst)


    def __enum_to_string_case(self):
        '''Generate the formated cases for converting the enum to a string.'''
        template = '''        case {0}:
            return "{1}";'''
        lst = [template.format(k, v) for k,v in self.enum_dict.items()]
        return '\n'.join(lst)


    def __pushback_all_values(self):
        '''Pushes back all different types into \"vec\"'''
        t = '        vec.push_back({0}({{0}}));'.format(self.classname)
        return '\n'.join([t.format(k) for k in sorted(self.enum_dict.keys())])
    

    def gen(self):
        '''Returns a string with the generated code.'''

        params = {
            'classname'   : self.classname,
            'enumtype'    : self.typename,
            'enumdefault' : self.enum_default,
            'string_to_enum_case' : self.__string_to_enum_case(),
            'enum_to_string_case' : self.__enum_to_string_case(),
            'pushback_all_values' : self.__pushback_all_values()
        }
        return(generator.__template.format(**params))


g = {'Compression':generator('Compression', 'pcg::OpenEXRIO::Compression',
                             'pcg::OpenEXRIO::None',
                             {'pcg::OpenEXRIO::None':'none',
                              'pcg::OpenEXRIO::RLE':'rle',
                              'pcg::OpenEXRIO::ZIPS':'zips',
                              'pcg::OpenEXRIO::ZIP':'zip',
                              'pcg::OpenEXRIO::PIZ':'piz',
                              'pcg::OpenEXRIO::PXR24':'pxr24',
                              'pcg::OpenEXRIO::B44':'b44',
                              'pcg::OpenEXRIO::B44A':'b44a'
                             }),
     'WriteChannels':generator('WriteChannels', 'pcg::OpenEXRIO::RgbaChannels',
                          'pcg::OpenEXRIO::WRITE_RGBA',
                          {'pcg::OpenEXRIO::WRITE_R':'r',
                           'pcg::OpenEXRIO::WRITE_G':'g',
                           'pcg::OpenEXRIO::WRITE_B':'b',
                           'pcg::OpenEXRIO::WRITE_A':'a',
                           'pcg::OpenEXRIO::WRITE_RGB':'rgb',
                           'pcg::OpenEXRIO::WRITE_RGBA':'rgba',
                           'pcg::OpenEXRIO::WRITE_YC':'yc',
                           'pcg::OpenEXRIO::WRITE_YCA':'yca',
                           'pcg::OpenEXRIO::WRITE_Y':'y',
                           'pcg::OpenEXRIO::WRITE_YA':'ya'
     })
     }

print(g['Compression'].gen())
print(g['WriteChannels'].gen())
