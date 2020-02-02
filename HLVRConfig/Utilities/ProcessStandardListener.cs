using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities
{
    class ProcessStandardListener
    {
        private Thread thread;
        public Process HLProcess { get; set; }

        public ProcessStandardListener(ThreadStart start)
        {
            thread = new Thread(start)
            {
                IsBackground = true
            };
            thread.Start();
        }

        public ProcessStandardListener(ParameterizedThreadStart start)
        {
            thread = new Thread(start)
            {
                IsBackground = true
            };
            thread.Start();
        }
    }
}
