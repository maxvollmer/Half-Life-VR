using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Documents;

namespace HLVRConfig.Utilities.UI.Controls
{
    public class I18NText : Run, II18NControl
    {
        public string Key { get; set; }

        private string defaultValue = "";

        public string DefaultValue
        {
            get
            {
                return defaultValue;
            }
            set
            {
                defaultValue = value;
                Update();
            }
        }

        public void Update()
        {
            base.Text = I18N.Get(new I18N.I18NString(Key, defaultValue));
        }
    }
}
