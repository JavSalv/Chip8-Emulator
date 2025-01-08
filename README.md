# CHIP-8 Emulator

Una implementación de un emulador CHIP-8 en C utilizando la librería de gráficos [SDL2](https://github.com/libsdl-org/SDL).
CHIP-8 es un lenguaje interpretado creado a mediados de los años 70, diseñado originalmente para el microordenador COSMAC VIP.

## 📸 Capturas de pantalla

### Space Invaders
![Space Invaders](./images/SpaceInvaders.png)

### Pong
![Pong](./images/Pong.png)

### Tetris
![Tetris](./images/Tetris.png)


## 🎮 Controles

El CHIP-8 original utiliza un teclado hexadecimal de 16 teclas. Las teclas están mapeadas de la siguiente forma:

```
Teclado Original | Teclado Actual
---------------------------------
    1 2 3 C          1 2 3 4
    4 5 6 D    ->    Q W E R
    7 8 9 E          A S D F
    A 0 B F          Z X C V
```

# ⚙️ Testing

El emulador ha sido testeado usando la [Suite de tests de Timendus](https://github.com/Timendus/chip8-test-suite).
Es capaz de pasar los tests relacionados con el set de instrucciones básico de Chip8.

![Tests](images/Test_suite.png)


## 🚀 Compilación e instalación

### Pasos para compilar

WIP

## 📝 TODOs

- [ ] Implementar soporte para variantes de Chip8 (SuperChipC, XO-Chip, etc.)
- [ ] Cargar configuración desde un fichero
- [ ] Mejorar la precisión del timing
- [ ] Añadir herramientas de debug (breakpoints, estado de registros, etc.)


## 📄 Licencia

Este proyecto está licenciado bajo la Licencia MIT - ver el archivo [LICENSE](LICENSE) para más detalles.

## 🔎 Referencias

- [Cowgod's Chip-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
- [Tobias V. Langhoff Guide](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)
- [Matt Mikolay Chip8 Documentation](https://github.com/mattmikolay/chip-8/wiki)
- [Timendus Chip8 Test Suite](https://github.com/Timendus/chip8-test-suite)
- [Chip8 Quirks](https://chip8.gulrak.net/)
- [Chip8 Quirks II](https://github.com/Chromatophore/HP48-Superchip)







