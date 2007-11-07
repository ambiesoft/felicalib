using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace FelicaLib
{
    public class FelicaLib
    {
	[DllImport("felicalib.dll")]
	private extern static IntPtr pasori_open(String dummy);
	[DllImport("felicalib.dll")]
	private extern static void pasori_close(IntPtr p);
	[DllImport("felicalib.dll")]
	private extern static int pasori_init(IntPtr p);
	[DllImport("felicalib.dll")]
	private extern static IntPtr felica_polling(IntPtr p, ushort systemcode, byte rfu, byte time_slot);
	[DllImport("felicalib.dll")]
	private extern static void felica_free(IntPtr f);
	[DllImport("felicalib.dll")]
	private extern static void felica_getidm(IntPtr f, byte[] data);
	[DllImport("felicalib.dll")]
	private extern static void felica_getpmm(IntPtr f, byte[] data);
	[DllImport("felicalib.dll")]
	private extern static int felica_read_without_encryption02(IntPtr f, int servicecode, int mode, byte addr, byte[] data);

	private IntPtr pasorip = IntPtr.Zero;
	private IntPtr felicap = IntPtr.Zero;

	public FelicaLib()
	{
	    pasorip = pasori_open(null);
	    if (pasorip == IntPtr.Zero)
	    {
		throw new Exception("can't load felicalib library");
	    }
	    if (pasori_init(pasorip) != 0)
	    {
		throw new Exception("can't connect PaSoRi");
	    }
	}

	~FelicaLib()
	{
	    pasori_close(pasorip);
	}

	public void Polling(int systemcode)
	{
	    felica_free(felicap);

	    felicap = felica_polling(pasorip, (ushort)systemcode, 0, 0);
	    if (felicap == IntPtr.Zero)
	    {
		throw new Exception("polling card failed");
	    }
	}

	public byte[] IDm()
	{
	    if (felicap == IntPtr.Zero)
	    {
		throw new Exception("no polling executed.");
	    }

	    byte[] buf = new byte[8];
	    felica_getidm(felicap, buf);
	    return buf;
	}    

	public byte[] PMm()
	{
	    if (felicap == IntPtr.Zero)
	    {
		throw new Exception("no polling executed.");
	    }

	    byte[] buf = new byte[8];
	    felica_getpmm(felicap, buf);
	    return buf;
	}    

	public byte[] ReadWithoutEncryption(int servicecode, int addr)
	{
	    if (felicap == IntPtr.Zero)
	    {
		throw new Exception("no polling executed.");
	    }

	    byte[] data = new byte[16];
	    int ret = felica_read_without_encryption02(felicap, servicecode, 0, (byte)addr, data);
	    if (ret != 0)
	    {
		return null;
	    }
	    return data;
	}
    }
}
