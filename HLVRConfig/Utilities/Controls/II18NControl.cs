using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities.Controls
{
    public interface II18NControl
    {
        string Key { get; set; }
        string DefaultValue { get; set; }
        void Update();
    }
}
