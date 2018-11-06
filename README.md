# Trabajo práctico Protocolos de Comunicación
### 2018C2 - Proxy POP3

### Integrantes del equipo:

 - Tomás Ferrer (Legajo 57207)
 - Felipe Gorostiaga (Legajo 57200)
 - Santiago Swinnen (Legajo 57258)

## Instrucciones

Clonado y compilación:
```
git clone git@bitbucket.org:itba/pc-2018b-09.git
cd pc-2018b-09
make
```

Esto generará en el mismo directorio los binarios:
 - `pop3filter` (cuyo código fuente esta en ./Proxy)
 - `pop3filterctl` (cuyo código fuente esta en ./Admin)
 - `stripmime` (cuyo código fuente esta en ./Stripmime)

### pop3filter

Las opciones de ejecución de pop3filter están presentes en pop3filter.8.
Ejecutar `man ./pop3filter.8`.

### stripmime

Para ejecutar `stripmime` por separado puede hacerse de la siguiente manera:

`FILTER_MEDIAS=<--MIME list--> ./stripmime < <--Email file--> > <--Output file-->`

También puede setearse la variable `FILTER_MSG` para cambiar el mensaje de filtrado.
El mensaje no debe contener la secuencia de caracteres `\r\n.\r\n`.

### pop3filterctl

Para ejecutar `pop3filterctl` es necesario que esté ejecutándose `pop3filter`.
Puede ejecutarse de la siguiente manera:

`./pop3filterctl` ó `./pop3filterctl <--Dirección IP--> <--Puerto-->`

La contraseña es `adminpass`.


Para más información, leer el archivo `Informe.pdf`.
