static const Char Regex0Str[] = {92, 100, 43, 45, 100, 97, 97, 103, 115, 91, 101, 93, 63, }; // "\d+-daags[e]?"
static const Char Regex1Str[] = {92, 100, 43, 45, 106, 97, 114, 105, 103, 91, 101, 93, 63, }; // "\d+-jarig[e]?"
static const Char Regex2Str[] = {92, 100, 43, 45, 107, 111, 112, 112, 105, 103, 91, 101, 93, 63, }; // "\d+-koppig[e]?"
static const Char Regex3Str[] = {40, 91, 48, 45, 49, 93, 92, 100, 124, 91, 50, 93, 91, 48, 45, 51, 93, 41, 92, 58, 91, 48, 45, 53, 93, 92, 100, 40, 92, 58, 91, 48, 45, 53, 93, 92, 100, 41, 63, }; // "([0-1]\d|[2][0-3])\:[0-5]\d(\:[0-5]\d)?"
static const Char Regex4Str[] = {40, 91, 48, 45, 49, 93, 92, 100, 124, 91, 50, 93, 91, 48, 45, 51, 93, 124, 92, 100, 41, 91, 92, 58, 92, 46, 93, 40, 91, 48, 45, 53, 93, 92, 100, 124, 92, 100, 41, 117, }; // "([0-1]\d|[2][0-3]|\d)[\:\.]([0-5]\d|\d)u"
static const Char Regex5Str[] = {40, 91, 48, 45, 49, 93, 92, 100, 124, 91, 50, 93, 91, 48, 45, 51, 93, 124, 92, 100, 41, 117, 40, 91, 48, 45, 53, 93, 92, 100, 124, 92, 100, 41, }; // "([0-1]\d|[2][0-3]|\d)u([0-5]\d|\d)"
static const Char Regex6Str[] = {92, 91, 92, 100, 123, 49, 44, 50, 125, 92, 93, }; // "\[\d{1,2}\]"
static const Char Regex7Str[] = {91, 35, 42, 92, 45, 43, 61, 93, 91, 35, 42, 92, 45, 43, 61, 93, 91, 35, 42, 92, 45, 43, 61, 93, 91, 35, 42, 92, 45, 43, 61, 93, 91, 35, 42, 92, 45, 43, 61, 93, 91, 35, 42, 92, 45, 43, 61, 93, 43, }; // "[#*\-+=][#*\-+=][#*\-+=][#*\-+=][#*\-+=][#*\-+=]+"
static const Char Regex8Str[] = {91, 92, 45, 92, 60, 92, 62, 92, 43, 93, 63, 40, 40, 92, 100, 123, 49, 44, 51, 125, 40, 92, 46, 92, 100, 123, 51, 125, 41, 41, 43, 124, 92, 100, 43, 41, 40, 91, 92, 46, 92, 44, 93, 92, 100, 43, 41, 63, }; // "[\-\<\>\+]?((\d{1,3}(\.\d{3}))+|\d+)([\.\,]\d+)?"
static const Char Regex9Str[] = {92, 100, 43, 91, 92, 46, 93, 92, 100, 43, }; // "\d+[\.]\d+"
static const Char Regex10Str[] = {92, 100, 43, 40, 101, 124, 100, 101, 124, 45, 115, 116, 101, 124, 115, 116, 101, 41, }; // "\d+(e|de|-ste|ste)"
static const Char Regex11Str[] = {40, 92, 45, 124, 41, 92, 100, 43, 40, 91, 92, 46, 92, 44, 93, 92, 100, 43, 41, 63, 40, 37, 124, 41, 63, 40, 92, 45, 92, 100, 43, 41, 63, 37, }; // "(\-|)\d+([\.\,]\d+)?(%|)?(\-\d+)?%"
static const Char Regex12Str[] = {92, 100, 43, }; // "\d+"
static const Char Regex13Str[] = {40, 105, 124, 118, 124, 120, 41, 123, 49, 44, 53, 125, 91, 92, 45, 93, 40, 105, 124, 118, 124, 120, 41, 123, 49, 44, 53, 125, }; // "(i|v|x){1,5}[\-](i|v|x){1,5}"
static const Char Regex14Str[] = {40, 105, 124, 118, 124, 120, 41, 123, 49, 44, 53, 125, }; // "(i|v|x){1,5}"
static const Char Regex15Str[] = {40, 105, 124, 118, 124, 120, 41, 123, 49, 44, 53, 125, 40, 92, 45, 41, 63, 40, 101, 41, }; // "(i|v|x){1,5}(\-)?(e)"
static const Char Regex16Str[] = {97, 97, 110, 103, 101, 46, 123, 51, 44, 125, 91, 114, 115, 102, 108, 109, 98, 93, 100, }; // "aange.{3,}[rsflmb]d"
static const Char Regex17Str[] = {97, 97, 110, 103, 101, 46, 123, 51, 44, 125, 91, 107, 102, 115, 104, 112, 93, 116, }; // "aange.{3,}[kfshp]t"
static const Char Regex18Str[] = {97, 102, 103, 101, 46, 123, 51, 44, 125, 91, 114, 115, 102, 108, 109, 98, 93, 100, }; // "afge.{3,}[rsflmb]d"
static const Char Regex19Str[] = {97, 102, 103, 101, 46, 123, 51, 44, 125, 91, 107, 102, 115, 104, 112, 93, 116, }; // "afge.{3,}[kfshp]t"
static const Char Regex20Str[] = {98, 105, 106, 101, 101, 110, 103, 101, 46, 123, 51, 44, 125, 91, 114, 115, 102, 108, 109, 98, 93, 100, }; // "bijeenge.{3,}[rsflmb]d"
static const Char Regex21Str[] = {98, 105, 106, 101, 101, 110, 103, 101, 46, 123, 51, 44, 125, 91, 107, 102, 115, 104, 112, 93, 116, }; // "bijeenge.{3,}[kfshp]t"
static const Char Regex22Str[] = {98, 105, 106, 103, 101, 46, 123, 51, 44, 125, 91, 114, 115, 102, 108, 109, 98, 93, 100, }; // "bijge.{3,}[rsflmb]d"
static const Char Regex23Str[] = {98, 105, 106, 103, 101, 46, 123, 51, 44, 125, 91, 107, 102, 115, 104, 112, 93, 116, }; // "bijge.{3,}[kfshp]t"
static const Char Regex24Str[] = {100, 111, 111, 114, 103, 101, 46, 123, 51, 44, 125, 91, 114, 115, 102, 108, 109, 98, 93, 100, }; // "doorge.{3,}[rsflmb]d"
static const Char Regex25Str[] = {100, 111, 111, 114, 103, 101, 46, 123, 51, 44, 125, 91, 107, 102, 115, 104, 112, 93, 116, }; // "doorge.{3,}[kfshp]t"
static const Char Regex26Str[] = {103, 101, 46, 123, 51, 44, 125, 101, 101, 114, 100, }; // "ge.{3,}eerd"
static const Char Regex27Str[] = {40, 91, 49, 45, 57, 93, 124, 48, 91, 49, 45, 57, 93, 124, 91, 49, 44, 50, 93, 92, 100, 124, 51, 91, 48, 44, 49, 93, 41, 92, 47, 40, 40, 48, 41, 63, 92, 100, 124, 49, 91, 49, 45, 50, 93, 41, }; // "([1-9]|0[1-9]|[1,2]\d|3[0,1])\/((0)?\d|1[1-2])"
static const Char Regex28Str[] = {40, 91, 49, 45, 57, 93, 124, 48, 91, 49, 45, 57, 93, 124, 91, 49, 44, 50, 93, 92, 100, 124, 51, 91, 48, 44, 49, 93, 41, 40, 40, 45, 40, 91, 49, 45, 57, 93, 124, 48, 91, 49, 45, 57, 93, 124, 91, 49, 93, 91, 48, 45, 50, 93, 41, 45, 41, 124, 40, 47, 40, 91, 49, 45, 57, 93, 124, 48, 91, 49, 45, 57, 93, 124, 91, 49, 93, 91, 48, 45, 50, 93, 41, 47, 41, 124, 40, 92, 46, 40, 91, 49, 45, 57, 93, 124, 48, 91, 49, 45, 57, 93, 124, 91, 49, 93, 91, 48, 45, 50, 93, 41, 92, 46, 41, 41, 91, 48, 45, 57, 93, 91, 48, 45, 57, 93, 40, 91, 48, 45, 57, 93, 91, 48, 45, 57, 93, 41, 63, }; // "([1-9]|0[1-9]|[1,2]\d|3[0,1])((-([1-9]|0[1-9]|[1][0-2])-)|(/([1-9]|0[1-9]|[1][0-2])/)|(\.([1-9]|0[1-9]|[1][0-2])\.))[0-9][0-9]([0-9][0-9])?"
static const Char Regex29Str[] = {40, 40, 91, 48, 45, 49, 93, 41, 63, 92, 100, 124, 91, 50, 93, 91, 48, 45, 51, 93, 41, 92, 46, 91, 48, 45, 53, 93, 92, 100, }; // "(([0-1])?\d|[2][0-3])\.[0-5]\d"

