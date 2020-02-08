using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities.Settings
{
    public interface ISettingsContainer
    {
        IEnumerable<KeyValuePair<string, Setting>> KeyValuePairs { get; }
        bool SetValue(string key, string value);
    }
}
