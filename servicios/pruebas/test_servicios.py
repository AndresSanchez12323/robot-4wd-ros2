"""Pruebas de los servicios. No hablan con el carro ni arrancan motores."""

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from comun.servicio import token_valido
from llm.logica import catalogo_valido, normalizar
from orquestador.logica import anotar, evaluar, llegada, nombre_visto, resumen_lidar, robot_util, seguir_busqueda
from vla.logica import decidir
from vlm.logica import extraer, gpu_exigida_lista, jpeg_valido

CATALOGO = [
    {"id": 1, "nombre": "gaseosa", "pistas": ["soda"]},
    {"id": 2, "nombre": "celular", "pistas": ["telefono"]},
    {"id": 3, "nombre": "bolsa", "pistas": ["bag"]},
]


class Token(unittest.TestCase):
    def test_solo_el_bearer_exacto(self):
        class Pedido:
            def __init__(self, valor):
                self.headers = {"Authorization": valor}
        self.assertTrue(token_valido(Pedido("Bearer secreto"), "secreto"))
        self.assertFalse(token_valido(Pedido("Bearer otro"), "secreto"))
        self.assertFalse(token_valido(Pedido("secreto"), "secreto"))


class Lenguaje(unittest.TestCase):
    def test_lista_en_orden_y_rechazo_fuera_del_catalogo(self):
        self.assertEqual(normalizar('{"lugares":[1, 2, 3]}', CATALOGO)["nombres"], ["gaseosa", "celular", "bolsa"])
        self.assertEqual(normalizar('```json\n{"lugares":["2"]}\n```', CATALOGO)["lugares"], [2])
        with self.assertRaises(ValueError):
            normalizar('{"lugares":[9]}', CATALOGO)
        with self.assertRaises(ValueError):
            normalizar('{"lugares":[]}', CATALOGO)
        self.assertFalse(catalogo_valido([{"id": 1, "nombre": "", "pistas": []}]))

    def test_el_archivo_conserva_los_lugares_y_agrega_objetos(self):
        archivo = Path(__file__).resolve().parents[1] / "orquestador" / "catalogo.json"
        catalogo = json.loads(archivo.read_text(encoding="utf-8"))
        self.assertTrue(catalogo_valido(catalogo))
        nombres = {lugar["id"]: lugar["nombre"] for lugar in catalogo}
        self.assertEqual(nombres[1], "cocina")
        self.assertEqual(nombres[2], "baño")
        self.assertEqual(nombres[3], "garaje")
        self.assertEqual(nombres[4], "salida")
        self.assertEqual(nombres[5], "gaseosa")
        self.assertEqual(nombres[6], "celular")
        self.assertEqual(nombres[7], "bolsa")
        self.assertEqual(nombres[16], "mochila")
        self.assertEqual(nombres[17], "cuaderno")
        self.assertEqual(nombres[23], "computador")
        self.assertEqual(nombres[27], "regla")
        self.assertEqual(nombres[28], "reloj")


class Accion(unittest.TestCase):
    def test_se_acerca_gira_o_busca(self):
        self.assertEqual(decidir("gaseosa", {"visible": True, "objeto": "gaseosa", "lado": "frente"},
                                 {"cerca": True, "frente_libre": False})["accion"], "parar")
        self.assertEqual(decidir("gaseosa", {"visible": True, "objeto": "gaseosa", "lado": "frente"},
                                 {"cerca": False, "frente_libre": False})["accion"], "avanzar")
        self.assertEqual(decidir("celular", {"visible": True, "objeto": "celular", "lado": "izquierda"},
                                 {"cerca": False, "frente_libre": True})["accion"], "girar_izquierda")
        self.assertEqual(decidir("bolsa", {"visible": False, "objeto": "ninguno", "lado": "indeterminada"},
                                 {"cerca": False, "frente_libre": True})["accion"], "explorar")
        self.assertEqual(decidir("gaseosa", {"visible": True, "objeto": "silla", "lado": "frente"},
                                 {"cerca": True})["accion"], "explorar")
        self.assertEqual(decidir("gafas", {"visible": True, "objeto": "gafas", "lado": "indeterminada"},
                                 {"cerca": True})["accion"], "explorar")
        self.assertEqual(llegada("avanzar", {"activa": False, "ultimaParada": "Parado al lado del objetivo"},
                                 "Ninguna", 1, False), "seguir")
        self.assertEqual(llegada("avanzar", {"activa": False, "ultimaParada": "Parado al lado del objetivo"},
                                 "Ninguna", 1, True), "llegue")

    def test_sin_objeto_explora_con_esquive(self):
        self.assertEqual(seguir_busqueda("explorar", 1, True, 0), ("explorar", 1, 1))
        self.assertEqual(seguir_busqueda("explorar", 1, False, 2), ("explorar", 1, 3))
        self.assertEqual(seguir_busqueda("avanzar", -1, True, 2), ("avanzar", -1, 0))
        self.assertEqual(seguir_busqueda("girar_derecha", 1, True, 1), ("girar_derecha", -1, 0))
        self.assertEqual(nombre_visto("gaseosa", "soda", [
            {"nombre": "gaseosa", "pistas": ["soda", "coca"]},
        ]), "gaseosa")
        self.assertEqual(nombre_visto("botella", "una botella de agua", [
            {"nombre": "botella", "pistas": ["bottle", "agua"]},
        ]), "botella")

    def test_la_bitacora_guarda_cada_decision_distinta(self):
        lista = []
        anotar(lista, {"estado": "en_marcha", "detalle": "Buscando botella", "lugar": "botella", "accion": "avanzar"})
        anotar(lista, {"estado": "en_marcha", "detalle": "Buscando botella", "lugar": "botella", "accion": "avanzar"})
        anotar(lista, {"estado": "en_marcha", "detalle": "Objetivo confirmado al frente", "lugar": "botella", "accion": "parar"})
        self.assertEqual(len(lista), 2)
        self.assertEqual(lista[1]["accion"], "parar")


class Vision(unittest.TestCase):
    def test_respuesta_estricta_y_gpu_obligatoria(self):
        visto = extraer('{"objeto":"gaseosa","lado":"frente","visible":true}')
        self.assertTrue(visto["visible"])
        confirmada = extraer('{"objeto":"botella","lado":"centro","visible":"si"}')
        self.assertTrue(confirmada["visible"])
        self.assertEqual(confirmada["lado"], "frente")
        with self.assertRaises(ValueError):
            extraer('{"objeto":"gaseosa","lado":"arriba","visible":true}')
        self.assertFalse(jpeg_valido("no-es-una-foto"))
        sano, _estado = gpu_exigida_lista(False, "")
        self.assertTrue(sano)
        sano, estado = gpu_exigida_lista(True, "")
        self.assertFalse(sano)
        self.assertFalse(estado["sano"])


class Mision(unittest.TestCase):
    def test_sin_un_servicio_no_esta_lista(self):
        completos = {nombre: {"sano": True} for nombre in ("llm", "vlm", "vla", "gpu", "robot")}
        self.assertTrue(evaluar(completos)["listo"])
        completos["vlm"] = None
        revision = evaluar(completos)
        self.assertFalse(revision["listo"])
        self.assertEqual(revision["faltan"], ["vlm"])

    def test_lidar_cerca_y_pared(self):
        mapa = [2000] * 72
        libre = resumen_lidar({"mapa": mapa, "frente": 0, "radio": 150, "horario": 1, "lidar": 1})
        self.assertTrue(libre["frente_libre"])
        self.assertFalse(libre["cerca"])
        mapa[0] = mapa[1] = 400
        pared = resumen_lidar({"mapa": mapa, "frente": 0, "radio": 150, "horario": 1, "lidar": 1})
        self.assertFalse(pared["frente_libre"])
        self.assertFalse(pared["cerca"])
        mapa[0] = mapa[1] = 200
        cerca = resumen_lidar({"mapa": mapa, "frente": 0, "radio": 150, "horario": 1, "lidar": 1})
        self.assertTrue(cerca["cerca"])

    def test_llegada_no_reutiliza_la_parada_anterior(self):
        estado = {"activa": 0, "ultimaParada": "Parado al lado del objetivo"}
        self.assertEqual(llegada("explorar", estado, "Parado al lado del objetivo", 1), "seguir")
        estado = {"activa": 0, "ultimaParada": "Parado al lado del objetivo"}
        self.assertEqual(llegada("explorar", estado, "Ninguna", 1, True), "llegue")
        self.assertEqual(llegada("parar", estado, "Ninguna", 0), "llegue")
        self.assertEqual(llegada("explorar", {"activa": 0, "ultimaParada": "ENCODER_RR_SIN_PULSOS"}, "Ninguna", 1), "fallo")

    def test_robot_sin_geometria_no_cuenta_como_listo(self):
        self.assertFalse(robot_util({"version": "1.6", "enlace": 1, "imu": 1, "lidar": 1, "configurada": 0, "permiso": 0}))
        self.assertTrue(robot_util({"version": "1.6", "enlace": 1, "imu": 1, "lidar": 1, "configurada": 1, "permiso": 1}))


if __name__ == "__main__":
    unittest.main()
