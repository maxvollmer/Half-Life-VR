using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace HLVRConfig.Utilities.CustomActions
{
    public class CustomAction
    {
        public class StringHolder
        {
            public string value = "";
        }

        public string name = "";
        public List<StringHolder> consolecommands = new List<StringHolder> { new StringHolder() };
        public List<FrameworkElement> uielemements = new List<FrameworkElement>();

        public CustomAction() {}
        public CustomAction(string name, IEnumerable<string> consolecommands)
        {
            this.name = name;
            this.consolecommands = consolecommands.Select(s => new StringHolder() { value = s }).ToList();
        }

        public void Write(StringBuilder customactionsfile)
        {
            foreach (var consolecommand in consolecommands)
            {
                customactionsfile.AppendLine($"{name} {consolecommand.value}");
            }
        }
    }
}
