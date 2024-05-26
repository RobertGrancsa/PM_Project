import {useState} from 'react'
import '../app/globals.css'
import {Dashboard} from "@/MainScreen.tsx";
import {createBrowserRouter, RouterProvider} from "react-router-dom";
import { ThemeProvider } from "@/components/theme-provider"


const router = createBrowserRouter([
  {
    path: "/",
    element: <Dashboard/>,
  },
]);

function App() {
  const [count, setCount] = useState(0)

  return (
      <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
        <RouterProvider router={router} />
      </ThemeProvider>
  )
}

export default App
