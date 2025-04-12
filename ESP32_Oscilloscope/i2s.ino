// Configures the I2S peripheral for ADC sampling at the specified rate
void configure_i2s(int rate) {
    /* Note:
       DMA constraint: dma_buf_len * dma_buf_count * bits_per_sample/8 > 4096
       Original code assumed includes from Arduino.h or other headers
    */
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),  // Master mode, receive, ADC input
        .sample_rate = rate,                                                          // Sample rate (e.g., 1000000 for 1 Msps)
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                                 // 16-bit samples (12-bit ADC data padded)
        .channel_format = I2S_CHANNEL_FMT_ALL_LEFT,                                   // Single left channel for ADC
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S),             // Updated to modern I2S format
        .intr_alloc_flags = 1,                                                        // Low-priority interrupt
        .dma_buf_count = 2,                                                           // 2 DMA buffers
        .dma_buf_len = NUM_SAMPLES,                                                   // 1000 samples per buffer
        .use_apll = 0                                                                // No Audio PLL
    };

    // Configure ADC1 channel (GPIO33) for 0–3.9 V range
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);  // Fixed: ADC_ATTEN_11db -> ADC_ATTEN_DB_11
    // Set 12-bit resolution
    adc1_config_width(ADC_WIDTH_BIT_12);  // Fixed: ADC_WIDTH_12Bit -> ADC_WIDTH_BIT_12

    // Install I2S driver
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    // Link I2S to ADC1, channel 5 (GPIO33)
    i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL);

    // Invert ADC data (hardware-specific, matches original behavior)
    SET_PERI_REG_MASK(SYSCON_SARADC_CTRL2_REG, SYSCON_SARADC_SAR1_INV);

    // Enable I2S ADC mode
    i2s_adc_enable(I2S_NUM_0);
}

// Samples ADC data into the buffer using I2S
void ADC_Sampling(uint16_t *i2s_buff) {
    size_t bytes_read;
    for (int i = 0; i < B_MULT; i++) {
        // Fixed: i2s_read_bytes -> i2s_read (modern API)
        // Reads 1000 samples (NUM_SAMPLES) into buffer at offset i*NUM_SAMPLES
        i2s_read(I2S_NUM_0, &i2s_buff[i * NUM_SAMPLES], NUM_SAMPLES * sizeof(uint16_t), 
                 &bytes_read, portMAX_DELAY);
        
        // Check for read errors (ensures reliable sampling)
        if (bytes_read != NUM_SAMPLES * sizeof(uint16_t)) {
            Serial.printf("I2S read error: expected %d bytes, got %d\n", 
                          NUM_SAMPLES * sizeof(uint16_t), bytes_read);
        }
    }
}

// Updates the I2S sampling rate
void set_sample_rate(uint32_t rate) {
    // Disable ADC mode before uninstalling driver
    i2s_adc_disable(I2S_NUM_0);
    // Remove current I2S driver
    i2s_driver_uninstall(I2S_NUM_0);
    // Reconfigure with new rate
    configure_i2s(rate);
}
