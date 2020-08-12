"""
Test for the SHTns C++ interface.
"""
import numpy as np
import matplotlib.pyplot as plt
import pytest
import scipy
from scatlib.sht import SHT
from scipy.special import roots_legendre, sph_harm

class TestSHT:
    """
    Testing of spherical harmonics transform for non-degenrate 2D fields.
    """
    def setup_method(self):
        self.l_max = 2 ** np.random.randint(4, 8)
        self.m_max = self.l_max
        self.n_lat = 2 * self.l_max
        self.n_lon = 4 * self.l_max
        self.sht = SHT(self.l_max, self.m_max, self.n_lat, self.n_lon, 1)
        self.lat_grid, _ = np.sort(np.arccos(roots_legendre(self.n_lat)))
        self.lon_grid = np.linspace(0, 2.0 * np.pi, self.n_lon + 1)[:-1]

    def test_latitude_grid(self):
        """
        Test that the SHT class returns the expected latitude grid (Gauss-Legendre).
        """
        lat_grid_ref = self.lat_grid
        lat_grid = self.sht.get_latitude_grid()
        assert np.all(np.isclose(lat_grid_ref, lat_grid))

    def test_colatitude_grid(self):
        """
        Test that the SHT class returns the expected co-latitude grid (Gauss-Legendre).
        """
        lat_grid_ref = np.cos(self.lat_grid)
        lat_grid = self.sht.get_colatitude_grid()
        print(lat_grid_ref, lat_grid)
        assert np.all(np.isclose(lat_grid_ref, lat_grid))

    def test_spatial_to_spectral(self):
        """
        Test transform from spatial to spectral representation by transforming
        spherical harmonics and ensuring that the result contains only one
        significant component.
        """
        l = np.random.randint(1, self.l_max)
        m = np.random.randint(-l, l)

        xx, yy = np.meshgrid(self.lat_grid, self.lon_grid, indexing="xy")
        zz = sph_harm(m, l, yy, xx) 
        coeffs = self.sht.transform(zz.real)

        assert np.sum(np.abs(coeffs) > 1e-6) == 1

    def test_inverse_transform(self):
        """
        Test transforming from spatial field to spectral field and back to ensure
        that the input field is recovered.
        """
        l = np.random.randint(1, self.l_max)
        m = np.random.randint(-l, l)

        xx, yy = np.meshgrid(self.lon_grid, self.lat_grid, indexing="ij")
        zz = sph_harm(m, l, xx, yy)
        coeffs = self.sht.transform(self.sht.transform(zz.real))
        assert np.all(np.isclose(zz.real, coeffs))

    def test_evaluate(self):
        """
        Test that evaluating the spectral representation reproduces the spatial
        input.
        """
        l = np.random.randint(1, self.l_max)
        m = np.random.randint(-l, l)

        xx, yy = np.meshgrid(self.lat_grid, self.lon_grid, indexing="xy")
        zz_ref = sph_harm(m, l, yy, xx) 
        coeffs = self.sht.transform(zz_ref.real)
        points = np.stack([yy.ravel(), xx.ravel()], axis=-1)
        zz = self.sht.evaluate(coeffs, points)

        print(m, l, self.n_lat, self.n_lon, self.l_max)

        assert np.all(np.isclose(zz, zz_ref.real.ravel()))
        return xx, yy, zz, zz_ref.real


class TestLegendreExpansion:
    """
    Testing of spherical harmonics transform for 1D fields.
    """
    def setup_method(self):
        self.l_max = 2 ** np.random.randint(4, 8)
        self.m_max = 0
        self.n_lat = 2 * self.l_max
        self.n_lon = 1
        self.sht = SHT(self.l_max, self.m_max, self.n_lat, self.n_lon, 1)

    def test_legendre_transform(self):
        """
        Tests the Legendre expansion by transforming a given Legendre polynomial
        and ensuring that only one component in the output is set.
        """
        l = np.random.randint(1, self.l_max)
        m = 0

        lat_grid, _ = np.arccos(roots_legendre(self.n_lat))
        lon_grid = np.linspace(0, 2.0 * np.pi, self.n_lon + 1)[:-1]
        xx, yy = np.meshgrid(lat_grid, lon_grid, indexing="xy")
        zz = sph_harm(m, l, yy, xx) 
        coeffs = self.sht.transform(zz.real)

        assert np.sum(np.abs(coeffs) > 1e-6) == 1