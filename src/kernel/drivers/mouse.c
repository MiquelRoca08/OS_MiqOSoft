#include <mouse.h>
#include <io.h>
#include <stdio.h>
#include <debug.h>

// Estado interno del driver
static bool mouse_initialized = false;
static bool mouse_has_wheel_support = false;
static uint8_t mouse_packet_buffer[4];
static int packet_pos = 0;
static int packet_size = 3; // 3 bytes para ratón estándar, 4 para ratón con rueda
static MousePacket current_packet;
static bool packet_ready = false;

// Callback para el scroll
void (*mouse_scroll_callback)(int8_t scroll_delta) = NULL;

// Función para esperar hasta que se pueda leer del ratón
static bool mouse_wait_read(void) {
    int timeout = 100000;
    while (timeout--) {
        if (i686_inb(MOUSE_STATUS_PORT) & 0x01) {
            return true;
        }
    }
    return false;
}

// Función para esperar hasta que se pueda escribir al ratón
static bool mouse_wait_write(void) {
    int timeout = 100000;
    while (timeout--) {
        if (!(i686_inb(MOUSE_STATUS_PORT) & 0x02)) {
            return true;
        }
    }
    return false;
}

// Escribir comando al controlador
static void mouse_write_controller(uint8_t command) {
    mouse_wait_write();
    i686_outb(MOUSE_COMMAND_PORT, command);
}

// Escribir comando al ratón
static void mouse_write_mouse(uint8_t data) {
    mouse_wait_write();
    i686_outb(MOUSE_COMMAND_PORT, MOUSE_CMD_WRITE_AUX);
    mouse_wait_write();
    i686_outb(MOUSE_DATA_PORT, data);
}

// Leer respuesta del ratón
static uint8_t mouse_read(void) {
    mouse_wait_read();
    return i686_inb(MOUSE_DATA_PORT);
}

// Detectar si el ratón tiene rueda
static bool mouse_detect_wheel(void) {
    log_info("MOUSE", "Detecting wheel support...");
    
    // Secuencia para activar la rueda (IntelliMouse)
    // Set sample rate to 200
    mouse_write_mouse(MOUSE_CMD_SET_SAMPLE);
    if (mouse_read() != MOUSE_ACK) return false;
    mouse_write_mouse(200);
    if (mouse_read() != MOUSE_ACK) return false;
    
    // Set sample rate to 100
    mouse_write_mouse(MOUSE_CMD_SET_SAMPLE);
    if (mouse_read() != MOUSE_ACK) return false;
    mouse_write_mouse(100);
    if (mouse_read() != MOUSE_ACK) return false;
    
    // Set sample rate to 80
    mouse_write_mouse(MOUSE_CMD_SET_SAMPLE);
    if (mouse_read() != MOUSE_ACK) return false;
    mouse_write_mouse(80);
    if (mouse_read() != MOUSE_ACK) return false;
    
    // Get device ID
    mouse_write_mouse(MOUSE_CMD_GET_ID);
    if (mouse_read() != MOUSE_ACK) return false;
    
    uint8_t id = mouse_read();
    log_info("MOUSE", "Device ID: 0x%02X", id);
    
    return (id == 0x03); // 0x03 = IntelliMouse (con rueda)
}

void mouse_init(void) {
    log_info("MOUSE", "Initializing PS/2 mouse...");
    
    // Habilitar el puerto auxiliar (ratón)
    mouse_write_controller(MOUSE_CMD_ENABLE_AUX);
    
    // Limpiar buffer de entrada
    while (i686_inb(MOUSE_STATUS_PORT) & 0x01) {
        i686_inb(MOUSE_DATA_PORT);
    }
    
    // Reset del ratón
    mouse_write_mouse(MOUSE_CMD_RESET);
    if (mouse_read() != MOUSE_ACK) {
        log_err("MOUSE", "Mouse reset failed");
        return;
    }
    
    // Leer los dos bytes de respuesta del reset (0xAA 0x00)
    mouse_read(); // Should be 0xAA
    mouse_read(); // Should be 0x00
    
    // Detectar si tiene rueda
    mouse_has_wheel_support = mouse_detect_wheel();
    if (mouse_has_wheel_support) {
        packet_size = 4;
        log_info("MOUSE", "Wheel mouse detected (4-byte packets)");
    } else {
        packet_size = 3;
        log_info("MOUSE", "Standard mouse detected (3-byte packets)");
    }
    
    // Configurar valores por defecto
    mouse_write_mouse(MOUSE_CMD_SET_DEFAULTS);
    if (mouse_read() != MOUSE_ACK) {
        log_warn("MOUSE", "Set defaults failed");
    }
    
    // Habilitar el ratón
    mouse_write_mouse(MOUSE_CMD_ENABLE);
    if (mouse_read() != MOUSE_ACK) {
        log_err("MOUSE", "Mouse enable failed");
        return;
    }
    
    // Habilitar interrupciones del ratón (IRQ 12)
    i686_outb(0x21, i686_inb(0x21) & ~(1 << 2)); // IRQ 12 está cascadeado a través de IRQ 2
    i686_outb(0xA1, i686_inb(0xA1) & ~(1 << 4)); // IRQ 12 en el PIC secundario
    
    mouse_initialized = true;
    log_info("MOUSE", "Mouse initialized successfully");
}

void mouse_handler(Registers* regs) {
    if (!mouse_initialized) return;
    
    // Verificar que hay datos disponibles
    if (!(i686_inb(MOUSE_STATUS_PORT) & 0x01)) return;
    
    uint8_t data = i686_inb(MOUSE_DATA_PORT);
    
    // Si es el primer byte, verificar que los bits de sincronización sean correctos
    if (packet_pos == 0) {
        // El primer byte debe tener el bit 3 establecido
        if (!(data & 0x08)) {
            return; // Descartar paquete mal sincronizado
        }
    }
    
    mouse_packet_buffer[packet_pos] = data;
    packet_pos++;
    
    // Si hemos recibido un paquete completo
    if (packet_pos >= packet_size) {
        packet_pos = 0;
        
        // Procesar el paquete
        current_packet.flags = mouse_packet_buffer[0];
        
        // Extraer las coordenadas X e Y con signo
        current_packet.x_delta = mouse_packet_buffer[1];
        current_packet.y_delta = mouse_packet_buffer[2];
        
        // Extender el signo si es necesario
        if (current_packet.flags & MOUSE_X_OVERFLOW) {
            current_packet.x_delta |= 0xFF00;
        }
        if (current_packet.flags & MOUSE_Y_OVERFLOW) {
            current_packet.y_delta |= 0xFF00;
        }
        
        // Extraer botones
        current_packet.left_button = (current_packet.flags & MOUSE_BUTTON_LEFT) != 0;
        current_packet.right_button = (current_packet.flags & MOUSE_BUTTON_RIGHT) != 0;
        current_packet.middle_button = (current_packet.flags & MOUSE_BUTTON_MIDDLE) != 0;
        
        // Procesar rueda si está disponible
        current_packet.z_delta = 0;
        if (mouse_has_wheel_support && packet_size == 4) {
            current_packet.z_delta = (int8_t)mouse_packet_buffer[3];
            
            // Si hay scroll y hay callback configurado, llamarlo
            if (current_packet.z_delta != 0 && mouse_scroll_callback) {
                mouse_scroll_callback(current_packet.z_delta);
            }
        }
        
        packet_ready = true;
        
        // Debug: mostrar información del ratón (descomenta para debug)
        /*
        if (current_packet.x_delta != 0 || current_packet.y_delta != 0 || current_packet.z_delta != 0) {
            log_debug("MOUSE", "dx=%d dy=%d dz=%d L=%d R=%d M=%d", 
                     current_packet.x_delta, current_packet.y_delta, current_packet.z_delta,
                     current_packet.left_button, current_packet.right_button, current_packet.middle_button);
        }
        */
    }
}

bool mouse_has_wheel(void) {
    return mouse_has_wheel_support;
}

MousePacket* mouse_get_packet(void) {
    if (packet_ready) {
        packet_ready = false;
        return &current_packet;
    }
    return NULL;
}

void mouse_set_scroll_callback(void (*callback)(int8_t scroll_delta)) {
    mouse_scroll_callback = callback;
}