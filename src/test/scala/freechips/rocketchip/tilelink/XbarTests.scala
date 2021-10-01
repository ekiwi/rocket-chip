package freechips.rocketchip.tilelink

import chipsalliance.rocketchip.config._
import org.scalatest.freespec.AnyFreeSpec
import chiseltest._
import chisel3._
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.subsystem.WithoutTLMonitors
class XbarTests extends AnyFreeSpec with ChiselScalatestTester {
  "simple xbar test" in {
    implicit val p: Parameters = new WithoutTLMonitors
    //implicit val p: Parameters = Parameters.empty
    test(new TLMulticlientXbarTest(nManagers = 2, nClients = 2)).withAnnotations(Seq(WriteFstAnnotation, VerilatorBackendAnnotation)) { dut =>
      dut.clock.setTimeout(0)
      dut.io.start.poke(true.B)
      dut.clock.step()
      dut.io.start.poke(false.B)
      dut.clock.step(150000)
    }
  }
}
