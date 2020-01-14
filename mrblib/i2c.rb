module PlatoESP32
  class I2C
    def read(cmds, len, type=:as_array)
      write(cmds)
      data = _read(len)
      if type == :as_array
        data = data.bytes
      end
      data
    end
  end
end

# registar ESP32 I2C device
Plato::I2C.register_device(PlatoESP32::I2C)
