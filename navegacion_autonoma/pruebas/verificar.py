"""Pruebas del firmware en PC. No abre puertos del robot ni envia movimiento."""
from pathlib import Path
import subprocess
import sys


def main():
    carpeta = Path(__file__).resolve().parent
    if sys.platform == 'win32':
        linux = '/mnt/' + carpeta.drive[0].lower() + '/' + carpeta.as_posix()[3:]
        prefijo = ['wsl', '-e', 'sh', '-c']
    else:
        linux = str(carpeta)
        prefijo = ['sh', '-c']
    script = '''set -e
cd "$1"
for t in protocolo navegacion motores casos_base integracion; do
  g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I dobles "${t}_test.cpp" -o "/tmp/robot_${t}_test"
  "/tmp/robot_${t}_test"
done
'''
    reporte = []
    for comando in (prefijo + [script, 'sh', linux],
                    ['node', str(carpeta / 'interfaz_test.cjs')]):
        r = subprocess.run(comando, capture_output=True, text=True)
        print(r.stdout, end='')
        print(r.stderr, end='', file=sys.stderr)
        reporte.append(r.stdout + r.stderr)
        if r.returncode:
            (carpeta / 'resultado_pruebas.txt').write_text(''.join(reporte), encoding='utf-8')
            return r.returncode
    (carpeta / 'resultado_pruebas.txt').write_text(''.join(reporte), encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
