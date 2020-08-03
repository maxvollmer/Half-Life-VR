using DbMon.NET;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities.Process
{
    class SingleProcessEnforcer
    {
        private readonly string mutexEventName = "HalfLifeVirtualRealityConfigMutexEventLaliludgnskdagjfgbs";
        private readonly string mutexName = "HalfLifeVirtualRealityConfigMutexLaliludgnskdagjfgbs";

        private EventWaitHandle mutexEventWaitHandle;
        private Mutex mutex;
        private bool isDisposed = false;

        public void ForceSingleProcess()
        {
            mutex = new Mutex(true, mutexName, out bool mutexCreated);
            mutexEventWaitHandle = new EventWaitHandle(false, EventResetMode.AutoReset, mutexEventName);
            if (!mutexCreated)
            {
                mutexEventWaitHandle.Set();
                throw new CancelAndTerminateAppException();
            }
            else
            {
                var thread = new Thread(() =>
                {
                    while (!isDisposed)
                    {
                        if (mutexEventWaitHandle.WaitOne(1000))
                        {
                            System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => (System.Windows.Application.Current?.MainWindow as MainWindow).BringToForeground()));
                        }
                    }
                })
                {
                    IsBackground = true
                };
                thread.Start();
            }
        }

        public void Dispose()
        {
            if (mutex != null)
            {
                mutex.ReleaseMutex();
                mutex.Close();
                mutex = null;
            }
            isDisposed = true;
        }

    }
}
